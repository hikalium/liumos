#pragma once

#include "paging.h"

template <typename VirtType, typename PhysType>
PhysType v2p(VirtType v) {
  return reinterpret_cast<PhysType>(
      liumos->kernel_pml4->v2p(reinterpret_cast<uint64_t>(v)));
}
