#include "hpet.h"

packed_struct RegisterSet {
  uint64_t configuration_and_capability;
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
  RegisterSet timers[32];
};

constexpr HPET::RegisterSpace::GeneralConfig operator|(
    HPET::RegisterSpace::GeneralConfig a,
    HPET::RegisterSpace::GeneralConfig b) {
  return static_cast<HPET::RegisterSpace::GeneralConfig>(
      static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

void HPET::Init(HPET::RegisterSpace* registers) {
  registers_ = registers;
  count_per_femtosecond_ = registers->general_capabilities_and_id >> 32;
  HPET::RegisterSpace::GeneralConfig general_config =
      registers->general_configuration;
  general_config =
      general_config |
      HPET::RegisterSpace::GeneralConfig::kUseLegacyReplacementRouting;
  general_config = general_config | HPET::RegisterSpace::GeneralConfig::kEnable;
  registers->general_configuration = general_config;
}
void HPET::SetTimerMs(int timer_index, uint64_t milliseconds, uint64_t flags) {
  uint64_t count = 1e12 * milliseconds / count_per_femtosecond_;
  RegisterSet* entry = &registers_->timers[timer_index];
  uint64_t config = entry->configuration_and_capability;
  uint64_t mask =
      HPET_MODE_PERIODIC | HPET_INT_ENABLE | HPET_INT_TYPE_LEVEL_TRIGGERED;
  config &= ~mask;
  config |= mask & flags;
  config |= HPET_SET_VALUE;
  entry->configuration_and_capability = config;
  entry->comparator_value = count;
  registers_->main_counter_value = 0;
}
