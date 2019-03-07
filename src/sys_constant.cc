#include "liumos.h"

uint64_t GetPhysAddrMask() {
  return liumos->cpu_features->phy_addr_mask;
}
