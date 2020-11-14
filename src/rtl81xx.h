#pragma once

#include <optional>

#include "generic.h"
#include "network.h"
#include "pci.h"

class RTL81 {
 public:
  void Init();
  static RTL81& GetInstance();

 private:
  static RTL81* rtl_;
  PCI::DeviceLocation dev_;
  uint16_t io_addr_base_;
};
