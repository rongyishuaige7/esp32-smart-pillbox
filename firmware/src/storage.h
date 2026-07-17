#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class StorageManager {
private:
    size_t recordLineCount_ = 0;

public:
    bool begin();
    bool savePlan(const String& json);
    String getPlan();
    bool saveRecord(const String& json);
    String getRecords();
    /// 分页：返回 {"total":N,"items":[...]} JSON 字符串
    String getRecordsPaged(size_t offset, size_t limit, size_t* outTotal);
    size_t getRecordLineCount();
    bool saveConfig(const String& json);
    String getConfig();
};

extern StorageManager storageManager;
#endif
