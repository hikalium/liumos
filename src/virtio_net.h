#pragma once

#include <optional>

#include "liumos.h"
#include "pci.h"

namespace Virtio {
class Net {
 public:
  void Init();

  static Net& GetInstance() {
    if (!net_) {
      net_ = liumos->kernel_heap_allocator->Alloc<Net>();
      bzero(net_, sizeof(Net));
      new (net_) Net();
    }
    assert(net_);
    return *net_;
  }

 private:
  static Net* net_;
  PCI::DeviceLocation dev_;
};
};  // namespace Virtio
