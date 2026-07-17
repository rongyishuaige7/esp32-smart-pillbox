#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <Arduino.h>

/// PIR + 称重联动
/// - take_ok: 拿起药瓶后放回，且放回后与 PIR 前基准的净重差 > kDoseThreshold（视为服药）
/// - take_and_return: 放回后净重差在阈值内（原样放回 / 未服药）
/// - open_no_take: PIR 后 10s 内无足够重量变化
/// - weight_without_motion: 无 PIR 窗口内称重突变

class BehaviorManager {
public:
  void begin();
  /// 每秒调用一次（与 main 中 1s 周期对齐）
  void tick();

private:
  struct CompState {
    uint8_t phase;  // 0 idle, 1 wait_weight, 2 wait_return
    unsigned long windowStartMs;
    /// 本次进入 phase1 时 PIR 上升沿时刻（用于双仓 PIR 串扰判定）
    unsigned long pirRisingEdgeMs;
    bool pirWasHigh;
    /// PIR 上升沿时的基准（带药稳定值）
    long lastStableWeight;
    /// 进入 phase2 时与 lastStableWeight 相同，用于判「放回」
    long baseBeforeTake;
    /// 检测到取药（重量明显减少）的时刻
    unsigned long takeDetectedMs;
    /// Phase 2 放回判定稳定计数：连续满足放回条件的 tick 次数
    int returnStableHits;
    /// 最近 3 次称重（用于 PIR 触发时取滑动均值作 base）
    long wHist0, wHist1, wHist2;
    uint8_t histCount;
  };

  CompState comp_[2];
  unsigned long bootStartMs_ = 0;

  void onBehaviorEvent(int compartment, const char* behaviorType);
  bool pirHigh(int compartment);
  long stableWeightSample(int compartment);

  static void advanceWeightHistory(CompState& s, long w);
  static long baseFromWeightHistory(const CompState& s);
};

extern BehaviorManager behaviorManager;

#endif
