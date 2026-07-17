#include "wifi_manager.h"
#include <WebServer.h>

Preferences preferences;

// This project deliberately uses an open provisioning AP. A fixed password
// would only look private while being published in the source. Provision only
// on an isolated, trusted test network and power the device off afterwards.
const char* ap_ssid = "SmartPillbox";

void WiFiManager::begin() {
    preferences.begin("wifi-config", false);
}

bool WiFiManager::connect() {
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");

    if (ssid.isEmpty()) {
        startAp();
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) return true;
    startAp();
    return false;
}

void WiFiManager::startAp() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid);
    Serial.println("Open provisioning AP started: " + WiFi.softAPIP().toString() +
                   " (trusted local setup only)");
}

void WiFiManager::saveConfig(const char* ssid, const char* password) {
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::hasSavedConfig() {
    String ssid = preferences.getString("ssid", "");
    return !ssid.isEmpty();
}

String WiFiManager::getIP() {
    if (WiFi.getMode() == WIFI_AP) return WiFi.softAPIP().toString();
    return WiFi.localIP().toString();
}

void WiFiManager::reset() {
    preferences.clear();
    WiFi.disconnect();
}

WiFiManager wifiManager;
