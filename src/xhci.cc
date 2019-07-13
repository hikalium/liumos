#include "xhci.h"

#include "liumos.h"

XHCI* XHCI::xhci_;

void XHCI::Init() {
  PutString("XHCI::Init()\n");

  is_found_ = false;
  for (auto& it : PCI::GetInstance().GetDeviceList()) {
    if (it.first != 0x000D'1B36)
      continue;
    is_found_ = true;
    dev_ = it.second;
    PutString("XHCI Controller Found: ");
    PutString(PCI::GetDeviceName(it.first));
    PutString("\n");
    break;
  }
  if (!is_found_) {
    PutString("XHCI Controller Not Found\n");
    return;
  }
}
