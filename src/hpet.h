#include "generic.h"

#define HPET_TIMER_CONFIG_REGISTER_BASE_OFS 0x100
#define HPET_TICK_PER_SEC (10UL * 1000 * 1000)

#define HPET_INT_TYPE_LEVEL_TRIGGERED (1UL << 1)
#define HPET_INT_ENABLE (1UL << 2)
#define HPET_MODE_PERIODIC (1UL << 3)
#define HPET_SET_VALUE (1UL << 6)

class HPET {
 public:
  struct RegisterSpace;
  void Init(RegisterSpace* registers);
  void SetTimerMs(int timer_index, uint64_t milliseconds, uint64_t flags);

 private:
  RegisterSpace* registers_;
  uint64_t count_per_femtosecond_;
};
