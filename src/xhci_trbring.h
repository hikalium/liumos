#pragma once

#include <cassert>
#include <cstdint>

#include "util.h"
#include "xhci_trb.h"

template <int N>
class TransferRequestBlockRing {
 public:
  static_assert(1 <= N && N <= 256);

  void Init(uint64_t paddr) {
    next_enqueue_idx_ = 0;
    current_cycle_state_ = 0;
    paddr_ = paddr;

    // 6.4.4.1 Link TRB
    // Table 6-91: TRB Type Definitions
    constexpr uint32_t kTRBTypeLink = 0x06;
    auto& link_trb = entry_[N];
    link_trb.data = paddr;
    link_trb.option = 0;
    link_trb.control = (kTRBTypeLink << 10) | 2;

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
  BasicTRB& GetEntryFromPhysAddr(uint64_t p) {
    uint64_t ofs = p - paddr_;
    assert(ofs % sizeof(BasicTRB) == 0);
    uint64_t index = ofs / sizeof(BasicTRB);
    assert(index < N);
    return entry_[index];
  }
  int GetCurrentCycleState() { return current_cycle_state_; }

 private:
  void AdvanceEnqueueIndexAndUpdateCycleBit() {
    volatile uint32_t& ctrl = entry_[next_enqueue_idx_].control;
    ctrl = (ctrl & ~0b1) | current_cycle_state_;
    next_enqueue_idx_++;
  }

  BasicTRB entry_[N + 1];
  int next_enqueue_idx_;
  int current_cycle_state_;
  uint64_t paddr_;
};
