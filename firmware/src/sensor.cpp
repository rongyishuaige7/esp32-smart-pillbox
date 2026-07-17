#include "sensor.h"
#include "firmware_config.h"

namespace {

static long median3(long a, long b, long c) {
  if (a > b) {
    long t = a;
    a = b;
    b = t;
  }
  if (b > c) {
    long t = b;
    b = c;
    c = t;
  }
  if (a > b) {
    long t = a;
    a = b;
    b = t;
  }
  return b;
}

}  // namespace

void SensorManager::begin(int pir1, int pir2, int dout, int sck, int dout2, int sck2) {
  pirPin1 = pir1;
  pirPin2 = pir2;
  hasSecondScale_ = (dout2 >= 0 && sck2 >= 0);

  pinMode(pirPin1, INPUT);
  pinMode(pirPin2, INPUT_PULLDOWN);

  scale1.begin(dout, sck);
  if (hasSecondScale_) {
    scale2.begin(dout2, sck2);
  }

  lastFilteredWeight_[0] = 0;
  lastFilteredWeight_[1] = 0;
  lastFilteredValid_[0] = false;
  lastFilteredValid_[1] = false;

  delay(FirmwareConfig::kHx711PowerUpDelayMs);
  calibrate(1);
  if (hasSecondScale_) {
    calibrate(2);
  }

  Serial.println("Sensor initialized (baseline locked for medication-on-power-on)");
}

bool SensorManager::isMotionDetected(int sensorNum) {
  if (sensorNum == 1) return digitalRead(pirPin1) == HIGH;
  if (sensorNum == 2) return digitalRead(pirPin2) == HIGH;
  return false;
}

long SensorManager::getRawWeight(int sensorNum) {
  if (sensorNum == 2 && !hasSecondScale_) return 0;
  int idx = sensorNum - 1;
  HX711& scale = (sensorNum == 1) ? scale1 : scale2;

  long samples[3];
  int got = 0;
  for (int i = 0; i < FirmwareConfig::kWeightMedianSampleCount; i++) {
    if (!scale.wait_ready_timeout(FirmwareConfig::kHx711ReadWaitReadyTimeoutMs)) {
      continue;
    }
    long raw = scale.read() - offset_[idx];
    samples[got++] = raw;
  }

  long m;
  if (got == 0) {
    if (lastFilteredValid_[idx]) {
      return lastFilteredWeight_[idx];
    }
    return 0;
  }
  if (got == 1) {
    m = samples[0];
  } else if (got == 2) {
    long a = samples[0];
    long b = samples[1];
    m = (a + b) / 2;
  } else {
    m = median3(samples[0], samples[1], samples[2]);
  }

  if (labs(m) < FirmwareConfig::kWeightDeadbandAbs) {
    m = 0;
  }

  lastFilteredWeight_[idx] = m;
  lastFilteredValid_[idx] = true;
  return m;
}

bool SensorManager::isWeightChanged(int sensorNum, long threshold) {
  if (sensorNum == 2 && !hasSecondScale_) return false;
  return labs(getRawWeight(sensorNum)) > threshold;
}

void SensorManager::calibrate(int sensorNum) {
  if (sensorNum == 2 && !hasSecondScale_) return;
  HX711& scale = (sensorNum == 1) ? scale1 : scale2;
  if (!scale.wait_ready_timeout(FirmwareConfig::kCalibrateWaitReadyTimeoutMs)) {
    Serial.print("Sensor ");
    Serial.print(sensorNum);
    Serial.println(" not responding!");
    offset_[sensorNum - 1] = 0;
    return;
  }

  long groups[FirmwareConfig::kCalibrateMedianGroupCount];
  for (int g = 0; g < FirmwareConfig::kCalibrateMedianGroupCount; g++) {
    groups[g] = scale.read_average(FirmwareConfig::kCalibrateReadAverageSamples);
    if (g < FirmwareConfig::kCalibrateMedianGroupCount - 1) {
      delay(FirmwareConfig::kCalibrateGroupDelayMs);
    }
  }

  int i = sensorNum - 1;
  offset_[i] = median3(groups[0], groups[1], groups[2]);

  Serial.print("[SENSOR] Sensor ");
  Serial.print(sensorNum);
  Serial.print(" baseline locked (median of 3 groups): ");
  Serial.println(offset_[i]);
}

SensorManager sensorManager;
