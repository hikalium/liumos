#include "rtl81xx.h"
#include "kernel.h"

RTL81* RTL81::rtl_;

RTL81& RTL81::GetInstance() {
  if (!rtl_) {
    rtl_ = liumos->kernel_heap_allocator->Alloc<RTL81>();
    bzero(rtl_, sizeof(RTL81));
    new (rtl_) RTL81();
  }
  assert(rtl_);
  return *rtl_;
}

static std::optional<PCI::DeviceLocation> FindRTL81XX() {
  for (auto& it : PCI::GetInstance().GetDeviceList()) {
    if (!it.first.HasID(0x10EC, 0x8139)) {
      continue;
    }
    PutString("Device Found: ");
    PutString(PCI::GetDeviceName(it.first));
    PutString("\n");
    return it.second;
  }
  PutString("Device Not Found\n");
  return {};
}

void RTL81::Init() {
  kprintf("RTL8::Init()\n");
  if (auto dev = FindRTL81XX()) {
    dev_ = *dev;
  } else {
    return;
  }
  PCI::EnsureBusMasterEnabled(dev_);
  PCI::BARForIO bar = PCI::GetBARForIO(dev_);
  io_addr_base_ = bar.base;
  PutStringAndHex("bar.base", bar.base);
}
