#include "reminder.h"
#include <string.h>

/*
 * RGB 极性（与模块「公共端」接法一致）：
 * - RGB_ACTIVE_HIGH=0（默认）：共阳，VCC 接 3V3，GPIO 拉低 = 亮。
 * - RGB_ACTIVE_HIGH=1：共阴，公共端接 GND，GPIO 拉高 = 亮（platformio.ini 加 -DRGB_ACTIVE_HIGH=1）
 *
 * 四脚顺序常见为 VCC,R,B,G：R→ledRPin，第 3 脚 B→ledBPin，第 4 脚 G→ledGPin。
 * 蓝色比红/绿更暗多为限流电阻偏大或正常现象，可适当减小 B 路电阻。
 */
#ifndef RGB_ACTIVE_HIGH
#define RGB_ACTIVE_HIGH 0
#endif

#define LEDC_CHANNEL 0
#define LEDC_RESOLUTION 8
#define REMINDER_BUZZER_HZ 2000

void ReminderManager::begin(int buzzer, int r, int g, int b) {
    buzzerPin = buzzer;
    ledRPin = r;
    ledGPin = g;
    ledBPin = b;
    isRinging = false;
    blinkActive = false;
    queueLen = 0;
    blinkR = 255;
    blinkG = 0;
    blinkB = 0;

    pinMode(buzzerPin, OUTPUT);
    pinMode(ledRPin, OUTPUT);
    pinMode(ledGPin, OUTPUT);
    pinMode(ledBPin, OUTPUT);

    digitalWrite(buzzerPin, LOW);
    setLED(0, 0, 0);
    ledcSetup(LEDC_CHANNEL, 2000, LEDC_RESOLUTION);
    ledcAttachPin(buzzerPin, LEDC_CHANNEL);

    Serial.println("Reminder module initialized");
}

void ReminderManager::bootSelfTest() {
    Serial.println("Boot self-test: RGB + buzzer 1.5s");
    const int flashMs = 200;
    const int gapMs = 150;
    Serial.printf("RGB pins: R=%d G=%d B=%d mode=%s\n", ledRPin, ledGPin, ledBPin,
                  RGB_ACTIVE_HIGH ? "ACTIVE_HIGH(共阴)" : "ACTIVE_LOW(共阳)");

    setLED(255, 0, 0);
    delay(400);
    setLED(0, 0, 0);
    delay(gapMs);

    setLED(0, 255, 0);
    delay(flashMs);
    setLED(0, 0, 0);
    delay(gapMs);

    setLED(0, 0, 255);
    delay(flashMs);
    setLED(0, 0, 0);
    delay(gapMs);

    ledcWriteTone(LEDC_CHANNEL, REMINDER_BUZZER_HZ);
    ledcWrite(LEDC_CHANNEL, 128);
    delay(1500);
    ledcWriteTone(LEDC_CHANNEL, 0);
    ledcWrite(LEDC_CHANNEL, 0);
    digitalWrite(buzzerPin, LOW);
}

void ReminderManager::startReminder() {
    isRinging = true;
}

void ReminderManager::stopReminder() {
    isRinging = false;
    blinkActive = false;
    ledcWriteTone(LEDC_CHANNEL, 0);
    ledcWrite(LEDC_CHANNEL, 0);
    digitalWrite(buzzerPin, LOW);
    setLED(0, 0, 0);
}

void ReminderManager::startBlinkWithRgb(int times, int delayMs, uint8_t r, uint8_t g, uint8_t b) {
    blinkR = r;
    blinkG = g;
    blinkB = b;
    isRinging = true;
    blinkActive = true;
    blinkTimesLeft = times * 2;
    blinkDelayMs = delayMs;
    blinkLastToggle = millis();
    blinkLedOn = true;
    setLED(blinkR, blinkG, blinkB);
    ledcWriteTone(LEDC_CHANNEL, REMINDER_BUZZER_HZ);
    ledcWrite(LEDC_CHANNEL, 128);
}

void ReminderManager::startBlink(int times, int delayMs) {
    startBlinkWithRgb(times, delayMs, 255, 0, 0);
}

void ReminderManager::startBlinkForCompartment(int compartment, int times, int delayMs) {
    if (compartment <= 1) {
        startBlinkWithRgb(times, delayMs, 255, 0, 0);
    } else {
        startBlinkWithRgb(times, delayMs, 0, 0, 255);
    }
}

void ReminderManager::enqueueCompartmentReminder(int compartment) {
    if (compartment < 1) compartment = 1;
    if (compartment > 2) compartment = 2;
    for (uint8_t i = 0; i < queueLen; i++) {
        if (compQueue[i] == (uint8_t)compartment) {
            return;
        }
    }
    if (queueLen >= 4) {
        return;
    }
    compQueue[queueLen++] = (uint8_t)compartment;
    if (!blinkActive && !isRinging) {
        tryStartNextQueued();
    }
}

void ReminderManager::tryStartNextQueued() {
    if (blinkActive || queueLen == 0) {
        return;
    }
    int c = (int)compQueue[0];
    memmove(compQueue, compQueue + 1, queueLen - 1);
    queueLen--;
    startBlinkForCompartment(c, 3, 500);
}

void ReminderManager::update() {
    if (!blinkActive && queueLen > 0) {
        tryStartNextQueued();
    }
    if (!blinkActive || blinkTimesLeft <= 0) {
        return;
    }
    unsigned long now = millis();
    if (now - blinkLastToggle < (unsigned long)blinkDelayMs) {
        return;
    }
    blinkLastToggle = now;
    blinkTimesLeft--;
    blinkLedOn = !blinkLedOn;
    if (blinkLedOn) {
        setLED(blinkR, blinkG, blinkB);
        ledcWriteTone(LEDC_CHANNEL, REMINDER_BUZZER_HZ);
        ledcWrite(LEDC_CHANNEL, 128);
    } else {
        setLED(0, 0, 0);
        ledcWriteTone(LEDC_CHANNEL, 0);
        ledcWrite(LEDC_CHANNEL, 0);
    }
    if (blinkTimesLeft <= 0) {
        stopReminder();
        if (queueLen > 0) {
            tryStartNextQueued();
        }
    }
}

void ReminderManager::playTone(int frequency, int duration) {
    if (frequency > 0) {
        ledcWriteTone(LEDC_CHANNEL, frequency);
        ledcWrite(LEDC_CHANNEL, 128);
        delay(duration);
        ledcWrite(LEDC_CHANNEL, 0);
    } else {
        ledcDetachPin(buzzerPin);
        digitalWrite(buzzerPin, HIGH);
        delay(duration);
        digitalWrite(buzzerPin, LOW);
        ledcAttachPin(buzzerPin, LEDC_CHANNEL);
    }
}

static inline void ledChannelWrite(int pin, bool lit) {
#if RGB_ACTIVE_HIGH
    digitalWrite(pin, lit ? HIGH : LOW);
#else
    digitalWrite(pin, lit ? LOW : HIGH);
#endif
}

void ReminderManager::setLED(uint8_t r, uint8_t g, uint8_t b) {
    ledChannelWrite(ledRPin, r > 0);
    ledChannelWrite(ledGPin, g > 0);
    ledChannelWrite(ledBPin, b > 0);
}

void ReminderManager::blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        setLED(255, 0, 0);
        delay(delayMs);
        setLED(0, 0, 0);
        delay(delayMs);
    }
}

bool ReminderManager::isActive() {
    return isRinging;
}

ReminderManager reminderManager;
