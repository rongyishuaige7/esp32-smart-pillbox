#include <Arduino.h>
#include <time.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"
#include "sensor.h"
#include "storage.h"
#include "reminder.h"
#include "webserver.h"
#include "behavior.h"
#include "firmware_config.h"
#include "pillbox_status.h"

// 漏服检测：时长见 FirmwareConfig::kMissedPirWindowMs
static bool waitingTake[2] = {false, false};
static unsigned long waitStartMs[2] = {0, 0};

void pillboxOnTakeOk(int compartment) {
  int i = compartment - 1;
  if (i >= 0 && i < 2) {
    waitingTake[i] = false;
  }
}

bool pillboxMissedPending(int compartment) {
  int i = compartment - 1;
  return i >= 0 && i < 2 && waitingTake[i];
}

static void armMissedWatch(int compartment) {
  int i = compartment - 1;
  if (i < 0 || i > 1) {
    return;
  }
  waitingTake[i] = true;
  waitStartMs[i] = millis();
}

static void saveMissedRecord(int compartment) {
  struct tm t;
  if (!getLocalTime(&t)) {
    return;
  }
  char buf[200];
  snprintf(buf, sizeof(buf),
           "{\"compartment\":%d,\"taken\":false,\"status\":\"missed\",\"behavior\":\"missed_dose\","
           "\"time\":\"%04d-%02d-%02d %02d:%02d:%02d\"}",
           compartment, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
  storageManager.saveRecord(String(buf));
}

static void checkMissedDoses() {
  for (int i = 0; i < 2; i++) {
    if (!waitingTake[i]) {
      continue;
    }
    if (sensorManager.isMotionDetected(i + 1)) {
      waitingTake[i] = false;
      continue;
    }
    if (millis() - waitStartMs[i] >= FirmwareConfig::kMissedPirWindowMs) {
      saveMissedRecord(i + 1);
      waitingTake[i] = false;
    }
  }
}

static void setupNtp() {
  configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
  setenv("TZ", "CST-8", 1);
  struct tm t;
  if (getLocalTime(&t, 5000)) {
    Serial.println("NTP synced");
  } else {
    Serial.println("NTP sync failed");
  }
}

static void checkPlanReminder() {
  struct tm t;
  if (!getLocalTime(&t)) {
    return;
  }
  int nowMin = t.tm_hour * 60 + t.tm_min;
  static int lastTriggeredMinute = -1;
  static int lastTriggeredYday = -1;
  static int lastTriggeredYear = -1;
  if (t.tm_year == lastTriggeredYear && t.tm_yday == lastTriggeredYday && nowMin == lastTriggeredMinute) {
    return;
  }

  String planJson = storageManager.getPlan();
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, planJson) != DeserializationError::Ok) {
    return;
  }
  JsonArray plans = doc["plans"];
  if (!plans) {
    return;
  }

  uint8_t seen = 0;
  bool any = false;
  for (JsonObject p : plans) {
    if (!p["enabled"].as<bool>()) {
      continue;
    }
    int compartment = p["compartment"] | 1;
    if (compartment < 1) {
      compartment = 1;
    }
    if (compartment > 2) {
      compartment = 2;
    }
    const char* times = p["times"].as<const char*>();
    if (!times) {
      continue;
    }
    String s(times);
    int start = 0;
    while (start < (int)s.length()) {
      int comma = s.indexOf(',', start);
      String part = comma >= 0 ? s.substring(start, comma) : s.substring(start);
      part.trim();
      if (part.length() >= 5 && part[2] == ':') {
        int h = part.substring(0, 2).toInt();
        int m = part.substring(3, 5).toInt();
        if (h * 60 + m == nowMin) {
          uint8_t mask = (uint8_t)(1 << (compartment - 1));
          if ((seen & mask) == 0) {
            seen |= mask;
            reminderManager.enqueueCompartmentReminder(compartment);
            armMissedWatch(compartment);
            any = true;
          }
        }
      }
      start = comma >= 0 ? comma + 1 : s.length();
    }
  }
  if (any) {
    lastTriggeredMinute = nowMin;
    lastTriggeredYday = t.tm_yday;
    lastTriggeredYear = t.tm_year;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Smart Pillbox Started");

  storageManager.begin();

  // 蜂鸣器=18；RGB：R=13 G=21 B=22。勿用 GPIO26/27（OLED I2C 占用）；勿用 27（ADC2+常为 SCL，红不亮/蓝发弱）
  reminderManager.begin(18, 13, 21, 22);
  reminderManager.bootSelfTest();

  wifiManager.begin();
  if (wifiManager.connect()) {
    Serial.println("WiFi Connected: " + wifiManager.getIP());
    setupNtp();
  } else {
    Serial.println("WiFi not connected; connect to AP SmartPillbox and open http://" + wifiManager.getIP() +
                   " to configure");
  }

  // PIR1=15, PIR2=25
  // HX711#1 药仓1 DT=4 SCK=2
  // HX711#2 药仓2 DT=33 SCK=32
  sensorManager.begin(15, 25, 4, 2, 33, 32);

  behaviorManager.begin();

  httpServer.begin();

  Serial.println("[SYSTEM] Calibration done. System ready.");
  Serial.println("[SYSTEM] Place medication in compartments BEFORE power-on; baseline = load at boot.");
  Serial.println("[SYSTEM] Behavior detection (PIR+weight) active after 5s boot guard.");
}

void loop() {
  httpServer.handle();
  reminderManager.update();
  checkMissedDoses();

  static unsigned long lastCheck = 0;
  static unsigned long lastWeightPrint = 0;

  if (millis() - lastWeightPrint > FirmwareConfig::kWeightDebugPrintIntervalMs) {
    lastWeightPrint = millis();
    long w1 = sensorManager.getRawWeight(1);
    bool pir1 = sensorManager.isMotionDetected(1);
    Serial.print("[W] 药仓1 weight=");
    Serial.print(w1);
    Serial.print("  PIR1=");
    Serial.print(pir1 ? "HIGH" : "low");
    if (sensorManager.hasSecondScale()) {
      long w2 = sensorManager.getRawWeight(2);
      bool pir2 = sensorManager.isMotionDetected(2);
      Serial.print("  | 药仓2 weight=");
      Serial.print(w2);
      Serial.print("  PIR2=");
      Serial.print(pir2 ? "HIGH" : "low");
    }
    Serial.println();
  }

  if (millis() - lastCheck > FirmwareConfig::kBehaviorTickIntervalMs) {
    lastCheck = millis();

    behaviorManager.tick();
    checkPlanReminder();

  }

  // 仅在 STA 模式（已存过 WiFi 配置）断线时才自动重连；
  // AP 配网模式下不重连，避免反复打印 ssid NOT_FOUND 日志。
  if (wifiManager.hasSavedConfig() && !wifiManager.isConnected()) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > FirmwareConfig::kWifiReconnectIntervalMs) {
      lastReconnect = millis();
      Serial.println("Reconnecting WiFi...");
      if (wifiManager.connect()) {
        Serial.println("WiFi reconnected: " + wifiManager.getIP());
        setupNtp();
      }
    }
  }

  delay(FirmwareConfig::kLoopDelayMs);
}
