#include "liumos.h"

using TimerConfig = HPET::TimerConfig;

packed_struct TimerRegister {
  TimerConfig configuration_and_capability;
  uint64_t comparator_value;
  uint64_t fsb_interrupt_route;
  uint64_t reserved;
};

namespace GeneralConfigBits {
constexpr uint64_t kEnable = 1 << 0;
constexpr uint64_t kUseLegacyReplacementRouting = 1 << 1;
};  // namespace GeneralConfigBits

namespace GeneralCapabilityBits {
constexpr uint64_t kMainCounterSupports64bit = 1 << 13;
uint8_t GetNumOfTimers(uint64_t cap) {
  return (cap >> 8) & 0b11111;
}
};  // namespace GeneralCapabilityBits

packed_struct HPET::RegisterSpace {
  uint64_t general_capabilities_and_id;
  uint64_t reserved00;
  uint64_t general_configuration;
  uint64_t reserved01;
  uint64_t general_interrupt_status;
  uint64_t reserved02;
  uint64_t reserved03[24];
  uint64_t main_counter_value;
  uint64_t reserved04;
  TimerRegister timers[32];
};

void HPET::Init(HPET::RegisterSpace* registers) {
  registers_ = registers;
  femtosecond_per_count_ = registers->general_capabilities_and_id >> 32;
  uint64_t general_config = registers->general_configuration;
  general_config |= GeneralConfigBits::kUseLegacyReplacementRouting;
  general_config |= GeneralConfigBits::kEnable;
  registers->general_configuration = general_config;
}

void HPET::SetTimerMs(int timer_index,
                      uint64_t milliseconds,
                      TimerConfig flags) {
  SetTimerNs(timer_index, 1e3 * milliseconds, flags);
}

void HPET::SetTimerNs(int timer_index, uint64_t ns, TimerConfig flags) {
  uint64_t count = 1e9 * ns / femtosecond_per_count_;
  TimerRegister* entry = &registers_->timers[timer_index];
  TimerConfig config = entry->configuration_and_capability;
  TimerConfig mask = TimerConfig::kUseLevelTriggeredInterrupt |
                     TimerConfig::kEnable | TimerConfig::kUsePeriodicMode;
  config &= ~mask;
  config |= mask & flags;
  config |= TimerConfig::kSetComparatorValue;
  entry->configuration_and_capability = config;
  entry->comparator_value = count;
  registers_->main_counter_value = 0;
}

uint64_t HPET::ReadMainCounterValue() {
  return GetKernelVirtAddrForPhysAddr(
             GetKernelVirtAddrForPhysAddr(this)->registers_)
      ->main_counter_value;
}

void HPET::BusyWait(uint64_t ms) {
  uint64_t count = 1'000'000'000'000ULL * ms / femtosecond_per_count_ +
                   ReadMainCounterValue();
  while (ReadMainCounterValue() < count)
    ;
}
uint64_t HPET::GetFemtosecndPerCount() {
  return GetKernelVirtAddrForPhysAddr(this)->femtosecond_per_count_;
}

void HPET::Print() {
  PutStringAndHex("HPET at", registers_);
  PutStringAndHex("  # of timers",
                  GeneralCapabilityBits::GetNumOfTimers(
                      registers_->general_capabilities_and_id));
  PutStringAndHex("  femtosecond_per_count", femtosecond_per_count_);
  PutStringAndBool("  main counter supports 64bit mode",
                   registers_->general_capabilities_and_id &
                       GeneralCapabilityBits::kMainCounterSupports64bit);
}
