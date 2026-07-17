#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <HX711.h>
#include "firmware_config.h"

class SensorManager {
private:
    int pirPin1, pirPin2;
    HX711 scale1;
    HX711 scale2;
    bool hasSecondScale_;
    long offset_[2] = {0, 0};
    /// 最近一次滤波后的重量（用于 !is_ready 时保持、避免 0 毛刺）
    long lastFilteredWeight_[2] = {0, 0};
    bool lastFilteredValid_[2] = {false, false};

public:
    void begin(int pir1, int pir2, int dout, int sck, int dout2 = -1, int sck2 = -1);
    bool isMotionDetected(int sensorNum);
    long getRawWeight(int sensorNum);
    bool isWeightChanged(int sensorNum, long threshold = FirmwareConfig::kWeightDeltaThreshold);
    void calibrate(int sensorNum);
    bool hasSecondScale() const { return hasSecondScale_; }
};

extern SensorManager sensorManager;
#endif
