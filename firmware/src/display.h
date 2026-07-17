#ifndef PILLBOX_DISPLAY_H
#define PILLBOX_DISPLAY_H

/// SSD1306 I2C（默认 SDA=26,SCL=27）。RGB 的 R 请勿接 26/27，否则与 OLED 冲突或 I2C 拉坏 LED。
void displayBegin();
void displayTick();

#endif
