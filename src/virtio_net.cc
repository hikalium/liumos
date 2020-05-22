#include "virtio_net.h"

namespace Virtio {

Net* Net::net_;

static std::optional<PCI::DeviceLocation> FindVirtioNet() {
  for (auto& it : PCI::GetInstance().GetDeviceList()) {
    if (it.first != 0x1000'1af4)
      continue;
    PutString("Device Found: ");
    PutString(PCI::GetDeviceName(it.first));
    PutString("\n");
    return it.second;
  }
  PutString("Device Not Found\n");
  return {};
}

void Net::Init() {
  PutString("Virtio::Net::Init()\n");

  if (auto dev = FindVirtioNet()) {
    dev_ = *dev;
  } else {
    return;
  }
  /*
  PCI::EnsureBusMasterEnabled(dev_);
  PCI::BAR64 bar0 = PCI::GetBAR64(dev_);

  cap_regs_ = MapMemoryForIO<CapabilityRegisters*>(bar0.phys_addr, bar0.size);
  */
}

}  // namespace Virtio
