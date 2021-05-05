#include "kernel.h"
#include "xhci.h"

/*

USB control flow:

Slot will be assigned after the device is plugged in.
if port_state_[port] == kSlotAssigned, the slot for a device is available.

With this port:
- Address Device

*/

void USBManager() {
  kprintf("USBManager started\n");
  XHCI::Controller::GetInstance().Init();
  for (;;) {
    XHCI::Controller::GetInstance().PollEvents();
    Sleep();
  }
}
