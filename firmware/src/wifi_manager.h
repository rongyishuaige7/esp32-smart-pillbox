#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
public:
    void begin();
    bool connect();
    void startAp();
    bool isConnected();
    bool hasSavedConfig();
    String getIP();
    void saveConfig(const char* ssid, const char* password);
    void reset();
};

extern WiFiManager wifiManager;
#endif
