#pragma once

#include <cstdint>

struct BasicTRB {
  uint64_t data;
  uint32_t option;
  uint32_t control;
};
static_assert(sizeof(BasicTRB) == 16);

template <int N>
class TransferRequestBlockRing {
 public:
  static_assert(1 <= N && N <= 256);

  void Init(uint64_t paddr) {
    next_enqueue_idx_ = 0;
    current_cycle_state_ = 1;

    // 6.4.4.1 Link TRB
    // Table 6-91: TRB Type Definitions
    constexpr uint32_t kTRBTypeLink = 0x06;
    auto& link_trb = entry_[N];
    link_trb.data = paddr;
    link_trb.option = 0;
    link_trb.control = kTRBTypeLink << 10;

    for (int i = 0; i < N; i++) {
      Push();
    }
  }
  int Push() {
    int consumer_cycle_state = current_cycle_state_;
    AdvanceEnqueueIndexAndUpdateCycleBit();
    if (next_enqueue_idx_ == N) {
      AdvanceEnqueueIndexAndUpdateCycleBit();
      next_enqueue_idx_ = 0;
      current_cycle_state_ = 1 - current_cycle_state_;
    }
    return consumer_cycle_state;
  }
  int GetNextEnqueueIndex() { return next_enqueue_idx_; }
  template <typename T>
  T GetNextEnqueueEntry() {
    return reinterpret_cast<T>(&entry_[next_enqueue_idx_]);
  }
  int GetCurrentCycleState() { return current_cycle_state_; }

 private:
  void AdvanceEnqueueIndexAndUpdateCycleBit() {
    uint32_t& ctrl = entry_[next_enqueue_idx_].control;
    ctrl = (ctrl & ~0b1) | current_cycle_state_;
    next_enqueue_idx_++;
  }

  BasicTRB entry_[N + 1];
  int next_enqueue_idx_;
  int current_cycle_state_;
};
