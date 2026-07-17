#include "behavior.h"
#include "firmware_config.h"
#include "sensor.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void pillboxOnTakeOk(int compartment);

using namespace FirmwareConfig;

void BehaviorManager::advanceWeightHistory(CompState& s, long w) {
  s.wHist2 = s.wHist1;
  s.wHist1 = s.wHist0;
  s.wHist0 = w;
  if (s.histCount < 3) {
    s.histCount++;
  }
}

long BehaviorManager::baseFromWeightHistory(const CompState& s) {
  if (s.histCount == 0) {
    return 0;
  }
  if (s.histCount == 1) {
    return s.wHist0;
  }
  if (s.histCount == 2) {
    return (s.wHist0 + s.wHist1) / 2;
  }
  return (s.wHist0 + s.wHist1 + s.wHist2) / 3;
}

void BehaviorManager::begin() {
  bootStartMs_ = millis();
  for (int i = 0; i < 2; i++) {
    comp_[i].phase = 0;
    comp_[i].windowStartMs = 0;
    comp_[i].pirRisingEdgeMs = 0;
    comp_[i].pirWasHigh = false;
    comp_[i].lastStableWeight = 0;
    comp_[i].baseBeforeTake = 0;
    comp_[i].takeDetectedMs = 0;
    comp_[i].returnStableHits = 0;
    comp_[i].wHist0 = comp_[i].wHist1 = comp_[i].wHist2 = 0;
    comp_[i].histCount = 0;
  }
}

bool BehaviorManager::pirHigh(int compartment) {
  return sensorManager.isMotionDetected(compartment);
}

long BehaviorManager::stableWeightSample(int compartment) {
  return sensorManager.getRawWeight(compartment);
}

void BehaviorManager::onBehaviorEvent(int compartment, const char* behaviorType) {
  struct tm t;
  if (!getLocalTime(&t)) {
    Serial.println("[BEHAVIOR] ERROR: cannot get local time, event not saved");
    return;
  }
  bool taken = (strcmp(behaviorType, "take_ok") == 0);

  Serial.print("[EVENT] Compartment ");
  Serial.print(compartment);
  Serial.print(" => ");
  if (strcmp(behaviorType, "take_ok") == 0) {
    Serial.println("take_ok  (after return: net weight loss >= dose threshold => taken)");
  } else if (strcmp(behaviorType, "take_and_return") == 0) {
    Serial.println("take_and_return  (after return: net change within threshold => not taken)");
  } else if (strcmp(behaviorType, "open_no_take") == 0) {
    Serial.println("open_no_take  (PIR but no take-level weight change in 10s)");
  } else if (strcmp(behaviorType, "weight_without_motion") == 0) {
    Serial.println("weight_without_motion  (weight changed without PIR => anomaly)");
  } else if (strcmp(behaviorType, "missed_dose") == 0) {
    Serial.println("missed_dose  (no PIR within 15s after reminder)");
  } else {
    Serial.println(behaviorType);
  }

  char buf[220];
  snprintf(buf, sizeof(buf),
           "{\"compartment\":%d,\"taken\":%s,\"behavior\":\"%s\",\"time\":\"%04d-%02d-%02d "
           "%02d:%02d:%02d\"}",
           compartment, taken ? "true" : "false", behaviorType, t.tm_year + 1900, t.tm_mon + 1,
           t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  storageManager.saveRecord(String(buf));
  if (taken) {
    pillboxOnTakeOk(compartment);
  }
}

void BehaviorManager::tick() {
  static unsigned long lastPirMs[2] = {0, 0};
  static long lastSampleW[2] = {0, 0};
  static bool first[2] = {true, true};
  static unsigned long lastBehaviorMs[2] = {0, 0};

    if (millis() - bootStartMs_ < kBootGuardMs) {
    for (int c = 1; c <= 2; c++) {
      int idx = c - 1;
      if (c == 2 && !sensorManager.hasSecondScale()) {
        continue;
      }
      CompState& s = comp_[idx];
      bool pir = pirHigh(c);
      long w = stableWeightSample(c);
      advanceWeightHistory(s, w);

      if (first[idx]) {
        lastSampleW[idx] = w;
        first[idx] = false;
      } else {
        lastSampleW[idx] = w;
      }
      if (pir) {
        lastPirMs[idx] = millis();
      }
      s.pirWasHigh = pir;
    }
    return;
  }

  for (int c = 1; c <= 2; c++) {
    int idx = c - 1;
    if (c == 2 && !sensorManager.hasSecondScale()) {
      continue;
    }
    CompState& s = comp_[idx];
    bool pir = pirHigh(c);
    long w = stableWeightSample(c);

    if (first[idx]) {
      lastSampleW[idx] = w;
      first[idx] = false;
    }

    if (pir) {
      lastPirMs[idx] = millis();
    }

    // PIR 上升沿检测：先用干净的历史算 base，再把当前 w 加入历史
    if (s.phase == 0 && pir && !s.pirWasHigh) {
      s.lastStableWeight = baseFromWeightHistory(s);
      advanceWeightHistory(s, w);
      s.phase = 1;
      s.windowStartMs = millis();
      s.pirRisingEdgeMs = millis();
      Serial.print("[BEHAVIOR] Compartment ");
      Serial.print(c);
      Serial.print(" PIR triggered => waiting for take (base=");
      Serial.print(s.lastStableWeight);
      Serial.print(", takeTh=");
      Serial.print(kWeightDeltaThreshold);
      Serial.print(", window=");
      Serial.print(kWaitWeightWindowMs / 1000);
      Serial.println("s)");
    } else {
      advanceWeightHistory(s, w);
    }
    s.pirWasHigh = pir;

    if (s.phase == 1) {
      // 实时串扰检测：
      // 情况A：兄弟仓在 phase 2 且两路 PIR 上升沿在同一窗口内（同时触发）
      // 情况B：兄弟仓已在 phase 2（正在等放回），此时本仓再次触发 PIR 必为误触
      if (sensorManager.hasSecondScale()) {
        int otherIdx = (idx == 0) ? 1 : 0;
        const CompState& other = comp_[otherIdx];
        if (other.phase == 2) {
          long edgeDelta = (long)s.pirRisingEdgeMs - (long)other.pirRisingEdgeMs;
          bool simultaneousTrigger = labs(edgeDelta) < (long)kPirCrosstalkWindowMs;
          // 情况B：本仓进入 phase 1 时兄弟仓已经在 phase 2（放药过程中的误触）
          // s.pirRisingEdgeMs > other.pirRisingEdgeMs 且 other 有效记录（非0）说明本仓是后触发的
          bool siblingAlreadyInPhase2 = (other.pirRisingEdgeMs != 0) &&
                                        (s.pirRisingEdgeMs > other.pirRisingEdgeMs);
          if (simultaneousTrigger || siblingAlreadyInPhase2) {
            Serial.print("[BEHAVIOR] Compartment ");
            Serial.print(c);
            Serial.println(" phase1 cancelled: sibling PIR crosstalk detected");
            s.phase = 0;
            lastSampleW[idx] = w;
            continue;
          }
        }
      }

      long dec = s.lastStableWeight - w;
      if (dec > kWeightDeltaThreshold) {
        s.phase = 2;
        s.baseBeforeTake = s.lastStableWeight;
        s.takeDetectedMs = millis();
        s.returnStableHits = 0;
        Serial.print("[BEHAVIOR] Compartment ");
        Serial.print(c);
        Serial.print(" take detected (dec=");
        Serial.print(dec);
        Serial.print(") => waitReturn ");
        Serial.print(kReturnWindowMs / 1000);
        Serial.println("s");
        lastSampleW[idx] = w;
        continue;
      }
      unsigned long elapsed = millis() - s.windowStartMs;
      if (elapsed > kWaitWeightWindowMs) {
        long delta = labs(w - s.lastStableWeight);
        Serial.print("[BEHAVIOR] Compartment ");
        Serial.print(c);
        Serial.print(" timeout (");
        Serial.print(elapsed / 1000);
        Serial.print("s), delta=");
        Serial.print(delta);
        Serial.println(" => open_no_take");
        onBehaviorEvent(c, "open_no_take");
        s.phase = 0;
        lastBehaviorMs[idx] = millis();
        lastSampleW[idx] = w;
        continue;
      }
      lastSampleW[idx] = w;
      continue;
    }

    if (s.phase == 2) {
      if (labs(w - s.baseBeforeTake) < kReturnThreshold) {
        s.returnStableHits++;
        if (s.returnStableHits >= kReturnStableCount) {
          long netChange = labs(s.baseBeforeTake - w);
          if (netChange > kDoseThreshold) {
            Serial.print("[BEHAVIOR] Compartment ");
            Serial.print(c);
            Serial.print(" bottle back (stable x");
            Serial.print(s.returnStableHits);
            Serial.print("), netChange=");
            Serial.print(netChange);
            Serial.print(" > doseTh=");
            Serial.print(kDoseThreshold);
            Serial.println(" => take_ok");
            onBehaviorEvent(c, "take_ok");
          } else {
            Serial.print("[BEHAVIOR] Compartment ");
            Serial.print(c);
            Serial.print(" bottle back (stable x");
            Serial.print(s.returnStableHits);
            Serial.print("), netChange=");
            Serial.print(netChange);
            Serial.print(" <= doseTh=");
            Serial.print(kDoseThreshold);
            Serial.println(" => take_and_return");
            onBehaviorEvent(c, "take_and_return");
          }
          s.phase = 0;
          lastBehaviorMs[idx] = millis();
          lastSampleW[idx] = w;
          continue;
        }
      } else {
        s.returnStableHits = 0;
      }
      if (millis() - s.takeDetectedMs > kReturnWindowMs) {
        Serial.print("[BEHAVIOR] Compartment ");
        Serial.print(c);
        Serial.print(" return window elapsed, no stable return => take_ok (now=");
        Serial.print(w);
        Serial.println(")");
        onBehaviorEvent(c, "take_ok");
        s.phase = 0;
        lastBehaviorMs[idx] = millis();
        lastSampleW[idx] = w;
        continue;
      }
      lastSampleW[idx] = w;
      continue;
    }

    // IDLE: phase == 0
    if (millis() - lastBehaviorMs[idx] < kIdleCooldownAfterBehaviorMs) {
      lastSampleW[idx] = w;
      continue;
    }
    if (millis() - lastPirMs[idx] < kNoPirForAnomalyMs) {
      lastSampleW[idx] = w;
      continue;
    }
    long step = w - lastSampleW[idx];
    if (labs(step) > kWeightDeltaThreshold) {
      Serial.print("[BEHAVIOR] Compartment ");
      Serial.print(c);
      Serial.print(" no PIR but weight step=");
      Serial.print(step);
      Serial.println(" => weight_without_motion");
      onBehaviorEvent(c, "weight_without_motion");
      lastBehaviorMs[idx] = millis();
    }
    lastSampleW[idx] = w;
  }
}

BehaviorManager behaviorManager;
