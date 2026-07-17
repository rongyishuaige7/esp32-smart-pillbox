#include "display.h"
#include "wifi_manager.h"
#include "reminder.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>

// 与 hardware/bom.md 中 RGB 占用的 21/22 错开
#define OLED_SDA 26
#define OLED_SCL 27

static U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
static bool displayOk = false;

void displayBegin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin();
  display.setContrast(200);
  displayOk = true;
  Serial.println("OLED SSD1306 ready (SDA=26 SCL=27)");
}

void displayTick() {
  if (!displayOk) {
    return;
  }
  struct tm t;
  char line1[24] = "Time: --:--:--";
  if (getLocalTime(&t)) {
    snprintf(line1, sizeof(line1), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  }

  char line2[28] = "IP: ---";
  String ip = wifiManager.getIP();
  if (ip.length() > 0 && ip.length() < sizeof(line2) - 4) {
    snprintf(line2, sizeof(line2), "IP:%s", ip.c_str());
  }

  const char* line3 = reminderManager.isActive() ? "Reminder: ON" : "Reminder: off";

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12, "Smart Pillbox");
  display.drawStr(0, 28, line1);
  display.drawStr(0, 44, line2);
  display.drawStr(0, 60, line3);
  display.sendBuffer();
}
