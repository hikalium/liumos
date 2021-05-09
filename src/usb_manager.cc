#include "kernel.h"
#include "xhci.h"

/*

USB control flow:

Slot will be assigned after the device is plugged in.
if port_state_[port] == kSlotAssigned, the slot for a device is available.

With this port:
- Address Device

*/

static void InitDrivers(XHCI::Controller& xhc) {
  (void)xhc;
}

void USBManager() {
  kprintf("USBManager started\n");
  auto& xhc = XHCI::Controller::GetInstance();
  xhc.Init();
  for (;;) {
    xhc.PollEvents();
    InitDrivers(xhc);
    Sleep();
  }
}
