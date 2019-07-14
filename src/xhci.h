#pragma once
#include "liumos.h"
#include "pci.h"

class XHCI {
 public:
  void Init();

  static XHCI& GetInstance() {
    if (!xhci_)
      xhci_ = new XHCI();
    assert(xhci_);
    return *xhci_;
  }

 private:
  packed_struct CapabilityRegisters {
    uint8_t length;
    uint8_t reserved;
    uint16_t version;
    uint32_t params[3];
    uint32_t cap_params1;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t cap_params2;
  };
  static_assert(sizeof(CapabilityRegisters) == 0x20);
  packed_struct OperationalRegisters {
    uint32_t command;
    uint32_t status;
    uint32_t page_size;
    uint32_t rsvdz1[2];
    uint32_t notification_ctrl;
    uint64_t cmd_ring_ctrl;
    uint64_t rsvdz2[2];
    uint64_t device_ctx_base_addr_array_ptr;
    uint64_t config;
  };
  static_assert(offsetof(OperationalRegisters, config) == 0x38);
  static XHCI* xhci_;
  bool is_found_;
  PCI::DeviceLocation dev_;
};
