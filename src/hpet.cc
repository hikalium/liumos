#include "hpet.h"

using TimerConfig = HPET::TimerConfig;

packed_struct TimerRegister {
  TimerConfig configuration_and_capability;
  uint64_t comparator_value;
  uint64_t fsb_interrupt_route;
  uint64_t reserved;
};

packed_struct HPET::RegisterSpace {
  uint64_t general_capabilities_and_id;
  uint64_t reserved00;
  enum class GeneralConfig : uint64_t {
    kEnable = 1 << 0,
    kUseLegacyReplacementRouting = 1 << 1,
  } general_configuration;
  uint64_t reserved01;
  uint64_t general_interrupt_status;
  uint64_t reserved02;
  uint64_t reserved03[24];
  uint64_t main_counter_value;
  uint64_t reserved04;
  TimerRegister timers[32];
};

using GeneralConfig = HPET::RegisterSpace::GeneralConfig;

constexpr GeneralConfig operator|=(GeneralConfig& a, GeneralConfig b) {
  a = static_cast<GeneralConfig>(static_cast<uint64_t>(a) |
                                 static_cast<uint64_t>(b));
  return a;
}

void HPET::Init(HPET::RegisterSpace* registers) {
  registers_ = registers;
  count_per_femtosecond_ = registers->general_capabilities_and_id >> 32;
  GeneralConfig general_config = registers->general_configuration;
  general_config |= GeneralConfig::kUseLegacyReplacementRouting;
  general_config |= GeneralConfig::kEnable;
  registers->general_configuration = general_config;
}

void HPET::SetTimerMs(int timer_index,
                      uint64_t milliseconds,
                      TimerConfig flags) {
  uint64_t count = 1e12 * milliseconds / count_per_femtosecond_;
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
