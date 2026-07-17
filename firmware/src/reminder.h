#ifndef REMINDER_H
#define REMINDER_H

#include <Arduino.h>

class ReminderManager {
private:
    int buzzerPin;
    int ledRPin, ledGPin, ledBPin;
    bool isRinging;
    // Non-blocking blink state
    int blinkTimesLeft;
    int blinkDelayMs;
    unsigned long blinkLastToggle;
    bool blinkLedOn;
    bool blinkActive;
    uint8_t blinkR, blinkG, blinkB;
    // 同一分钟多个计划：按药仓排队提醒
    uint8_t compQueue[4];
    uint8_t queueLen;
    void startBlinkWithRgb(int times, int delayMs, uint8_t r, uint8_t g, uint8_t b);
    void tryStartNextQueued();

public:
    void begin(int buzzer, int r, int g, int b);
    /// 开机自检：RGB 三色依次闪烁，蜂鸣器响 1.5s（阻塞，仅 setup 中调用）
    void bootSelfTest();
    void startReminder();
    void stopReminder();
    void startBlink(int times, int delayMs);  // non-blocking; call update() from loop
    /// 按药仓颜色提醒：1=红，2=蓝
    void startBlinkForCompartment(int compartment, int times, int delayMs);
    void enqueueCompartmentReminder(int compartment);
    void update();  // call every loop() to drive non-blocking blink
    void playTone(int frequency, int duration);
    void setLED(uint8_t r, uint8_t g, uint8_t b);
    void blinkLED(int times, int delayMs);  // blocking, avoid in request handlers
    bool isActive();
};

extern ReminderManager reminderManager;
#endif
