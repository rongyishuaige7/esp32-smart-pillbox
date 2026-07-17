#ifndef FIRMWARE_CONFIG_H
#define FIRMWARE_CONFIG_H

#include <Arduino.h>

/// 固件可调参数集中配置（修改后需重新编译烧录）
namespace FirmwareConfig {

// ---------------------------------------------------------------------------
// 行为检测（PIR + 称重状态机）— 见 behavior.cpp
// ---------------------------------------------------------------------------

/// PIR 触发后，在此时间内若未出现「拿起药瓶」级重量减少，则记为 open_no_take（开盖未取），单位 ms
constexpr unsigned long kWaitWeightWindowMs = 10000;

/// 相对 PIR 时基准，重量读数减少超过此值（HX711 相对计数）视为「已拿起药瓶」并进入 phase2
constexpr long kWeightDeltaThreshold = 500;

/// 无 PIR 时，若距离上次行为事件不足此时长，则不检测 weight_without_motion，避免误报，单位 ms
constexpr unsigned long kIdleCooldownAfterBehaviorMs = 2500;

/// 无 PIR 时，若距离上次 PIR 活动不足此时长，则不检测无 PIR 异常，单位 ms
constexpr unsigned long kNoPirForAnomalyMs = 4000;

/// 上电后在此时间内只做传感器同步、不判定服药行为，单位 ms
constexpr unsigned long kBootGuardMs = 5000;

/// 拿起药瓶后，在此时间内等待放回；超时未稳定放回则记 take_ok，单位 ms
constexpr unsigned long kReturnWindowMs = 30000;

/// 判定「药瓶已放回托盘」：当前读数与 PIR 前基准之差的绝对值小于此值（HX711 计数）
/// 适当加大以容忍死区/漂移（例如基准近 0、放回后约 -1000 仍算已放回）；可再调到 1200～2000
constexpr long kReturnThreshold = 2000;

/// 放回后与 PIR 前基准的净重差绝对值超过此值（HX711 计数）视为「按时服药」take_ok，否则为 take_and_return
constexpr long kDoseThreshold = 500;

/// Phase 2 放回判定：要求连续 N 次 tick 读数都在 kReturnThreshold 范围内才确认放回，
/// 防止 HX711 回弹震荡时读数经过误判区间就立刻下结论
constexpr int kReturnStableCount = 3;

/// 两路 PIR 上升沿时间差在此以内视为「同时触发」（安装过近串扰）；phase1 超时且另一仓已 phase2 时取消本仓 open_no_take，单位 ms
constexpr unsigned long kPirCrosstalkWindowMs = 5000;

// ---------------------------------------------------------------------------
// 漏服检测 — 见 main.cpp
// ---------------------------------------------------------------------------

/// 用药提醒触发后，在此时间内若该药仓 PIR 从未触发，则记 missed_dose，单位 ms
constexpr unsigned long kMissedPirWindowMs = 15000;

// ---------------------------------------------------------------------------
// 主循环节奏 — 见 main.cpp
// ---------------------------------------------------------------------------

/// 串口打印 [W] 称重/PIR 的间隔，单位 ms
constexpr unsigned long kWeightDebugPrintIntervalMs = 500;

/// behaviorManager.tick()、用药计划、漏服检查的执行周期，单位 ms
constexpr unsigned long kBehaviorTickIntervalMs = 1000;

/// STA 模式下 WiFi 断线后自动重连尝试间隔，单位 ms
constexpr unsigned long kWifiReconnectIntervalMs = 30000;

/// loop() 末尾 delay，避免看门狗/空转占满 CPU，单位 ms
constexpr unsigned long kLoopDelayMs = 10;

// ---------------------------------------------------------------------------
// HX711 / 传感器 — 见 sensor.cpp
// ---------------------------------------------------------------------------

/// scale.begin() 之后等待 ADC 稳定再标定，单位 ms
constexpr unsigned long kHx711PowerUpDelayMs = 2000;

/// getRawWeight 每次 median 采样时 wait_ready 超时，单位 ms
constexpr unsigned long kHx711ReadWaitReadyTimeoutMs = 120;

/// getRawWeight 连续读几次再取中位数（当前 sensor.cpp 仅实现 3 点中位数，勿改除非同步改采样逻辑）
constexpr int kWeightMedianSampleCount = 3;

/// 滤波死区：|读数| 小于此值（减 offset 后）视为 0，抑制零漂，单位 HX711 计数
constexpr long kWeightDeadbandAbs = 200;

/// calibrate() 时 wait_ready 超时，单位 ms
constexpr unsigned long kCalibrateWaitReadyTimeoutMs = 2000;

/// calibrate() 每组 read_average 的采样次数
constexpr int kCalibrateReadAverageSamples = 10;

/// calibrate() 三组采样之间的间隔，单位 ms
constexpr unsigned long kCalibrateGroupDelayMs = 50;

/// calibrate() 取几组 read_average 再取中位数作为 offset（当前为 3）
constexpr int kCalibrateMedianGroupCount = 3;

}  // namespace FirmwareConfig

#endif
