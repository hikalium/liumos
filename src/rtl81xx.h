#pragma once

#include <optional>

#include "generic.h"
#include "network.h"
#include "pci.h"

class RTL81 {
 public:
  void Init();
  static RTL81& GetInstance();

  packed_struct RXCommandDescriptor {
    volatile uint32_t buf_size_and_flag;
    volatile uint32_t vlan_info;
    volatile uint64_t buf_phys_addr;
  };
  static_assert(sizeof(RXCommandDescriptor) == 16);

 private:
  uint16_t ReadPHYReg(uint8_t addr);

  static RTL81* rtl_;
  PCI::DeviceLocation dev_;
  uint16_t io_addr_base_;
  static constexpr uint32_t kSizeOfEachRXBuffer = 4096;
  static constexpr uint32_t kRXFlagsOwnedByController = (1 << 31);
  static constexpr uint32_t kRXFlagsEndOfRing = (1 << 30);
  static constexpr int kNumOfRXDescriptors = 32;
  RXCommandDescriptor* rx_descriptors_;
  volatile void* rx_buffers_[kNumOfRXDescriptors];
};
