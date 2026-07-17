#include "storage.h"
#include <vector>

#define MAX_RECORDS 200

static const char* kRecordsNdjson = "/records.ndjson";
static const char* kRecordsLegacy = "/records.json";

namespace {

void migrateLegacyRecordsIfNeeded() {
    if (SPIFFS.exists(kRecordsNdjson)) {
        return;
    }
    if (!SPIFFS.exists(kRecordsLegacy)) {
        return;
    }
    File oldFile = SPIFFS.open(kRecordsLegacy, FILE_READ);
    if (!oldFile) {
        return;
    }
    String content = oldFile.readString();
    oldFile.close();

    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        Serial.println("migrateLegacyRecords: invalid legacy JSON, skipping");
        return;
    }
    if (!doc.is<JsonArray>()) {
        SPIFFS.remove(kRecordsLegacy);
        return;
    }
    JsonArray arr = doc.as<JsonArray>();
    File out = SPIFFS.open(kRecordsNdjson, FILE_WRITE);
    if (!out) {
        Serial.println("migrateLegacyRecords: cannot create ndjson");
        return;
    }
    for (JsonObject item : arr) {
        String line;
        serializeJson(item, line);
        out.println(line);
    }
    out.close();
    SPIFFS.remove(kRecordsLegacy);
    Serial.println("Migrated records.json -> records.ndjson");
}

static size_t countLinesInNdjson() {
    File f = SPIFFS.open(kRecordsNdjson, FILE_READ);
    if (!f) {
        return 0;
    }
    size_t n = 0;
    while (f.available()) {
        f.readStringUntil('\n');
        n++;
    }
    f.close();
    return n;
}

void trimNdjsonToMax() {
    File in = SPIFFS.open(kRecordsNdjson, FILE_READ);
    if (!in) {
        return;
    }
    std::vector<String> lines;
    while (in.available()) {
        String line = in.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            lines.push_back(line);
        }
    }
    in.close();
    while (lines.size() > MAX_RECORDS) {
        lines.erase(lines.begin());
    }
    File out = SPIFFS.open(kRecordsNdjson, FILE_WRITE);
    if (!out) {
        return;
    }
    for (const String& l : lines) {
        out.println(l);
    }
    out.close();
}

}  // namespace

bool StorageManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return false;
    }
    Serial.println("SPIFFS initialized");
    migrateLegacyRecordsIfNeeded();
    recordLineCount_ = countLinesInNdjson();
    return true;
}

bool StorageManager::savePlan(const String& json) {
    File file = SPIFFS.open("/plan.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open plan.json for writing");
        return false;
    }
    file.print(json);
    file.close();
    return true;
}

String StorageManager::getPlan() {
    File file = SPIFFS.open("/plan.json", FILE_READ);
    if (!file) {
        return "{}";
    }
    String content = file.readString();
    file.close();
    return content;
}

bool StorageManager::saveRecord(const String& json) {
    String line = json;
    line.replace("\n", "");
    line.replace("\r", "");
    if (line.length() == 0) {
        return false;
    }

    File file = SPIFFS.open(kRecordsNdjson, FILE_APPEND);
    if (!file) {
        file = SPIFFS.open(kRecordsNdjson, FILE_WRITE);
    }
    if (!file) {
        Serial.println("Failed to open records.ndjson for writing");
        return false;
    }
    file.println(line);
    file.close();

    recordLineCount_++;
    if (recordLineCount_ > MAX_RECORDS) {
        trimNdjsonToMax();
        recordLineCount_ = MAX_RECORDS;
    }
    return true;
}

String StorageManager::getRecords() {
    File file = SPIFFS.open(kRecordsNdjson, FILE_READ);
    if (!file) {
        File legacy = SPIFFS.open(kRecordsLegacy, FILE_READ);
        if (!legacy) {
            return "[]";
        }
        String content = legacy.readString();
        legacy.close();
        return content;
    }

    String result = "[";
    bool first = true;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
            continue;
        }
        if (!first) {
            result += ",";
        }
        result += line;
        first = false;
    }
    file.close();
    result += "]";
    return result;
}

size_t StorageManager::getRecordLineCount() {
    return recordLineCount_;
}

String StorageManager::getRecordsPaged(size_t offset, size_t limit, size_t* outTotal) {
    File file = SPIFFS.open(kRecordsNdjson, FILE_READ);
    if (!file) {
        File legacy = SPIFFS.open(kRecordsLegacy, FILE_READ);
        if (!legacy) {
            if (outTotal) {
                *outTotal = 0;
            }
            return "{\"total\":0,\"items\":[]}";
        }
        String content = legacy.readString();
        legacy.close();
        if (outTotal) {
            *outTotal = 1;
        }
        return String("{\"total\":1,\"items\":") + content + "}";
    }

    std::vector<String> lines;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            lines.push_back(line);
        }
    }
    file.close();

    size_t total = lines.size();
    if (outTotal) {
        *outTotal = total;
    }
    // Newest records are appended last; return pages from newest to oldest.
    // offset=0: last `limit` lines; offset grows: skip that many newest, then take `limit`.
    if (offset >= total) {
        return String("{\"total\":") + String(total) + ",\"items\":[]}";
    }
    size_t first_idx = total - 1 - offset;
    size_t available = first_idx + 1;
    size_t count = limit < available ? limit : available;

    String result = "{\"total\":" + String(total) + ",\"items\":[";
    bool first = true;
    for (size_t k = 0; k < count; k++) {
        size_t idx = first_idx - k;
        if (!first) {
            result += ",";
        }
        result += lines[idx];
        first = false;
    }
    result += "]}";
    return result;
}

bool StorageManager::saveConfig(const String& json) {
    File file = SPIFFS.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open config.json for writing");
        return false;
    }
    file.print(json);
    file.close();
    return true;
}

String StorageManager::getConfig() {
    File file = SPIFFS.open("/config.json", FILE_READ);
    if (!file) {
        return "{}";
    }
    String content = file.readString();
    file.close();
    return content;
}

StorageManager storageManager;
