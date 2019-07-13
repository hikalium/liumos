#pragma once
#include "pci.h"

class XHCI {
 public:
  void Init();

  static XHCI* GetInstance() {
    if (!xhci_)
      xhci_ = new XHCI();
    return xhci_;
  }

 private:
  static XHCI* xhci_;
  bool is_found_;
  PCI::DeviceLocation dev_;
};
