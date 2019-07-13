#include "pci.h"

#include "liumos.h"

#include <cstdio>
#include <string>

PCI* PCI::pci_;

static const std::unordered_multimap<uint32_t, const char*> device_infos = {
    {0x000D'1B36, "QEMU XHCI Host Controller"},
    {0x2918'8086, "82801IB (ICH9) LPC Interface Controller"},
    {0x29c0'8086, "82G33/G31/P35/P31 Express DRAM Controller"},
    {0x1111'1234, "QEMU Virtual Video Controller"},
    {0x8168'10ec, "RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller"},
};

static uint32_t ReadPCIRegister(uint32_t bus,
                                uint32_t device,
                                uint32_t func,
                                uint32_t reg) {
  assert((bus & ~0b1111'1111) == 0);
  assert((device & ~0b1'1111) == 0);
  assert((func & ~0b111) == 0);
  assert((reg & ~0b1111'1100) == 0);
  constexpr uint16_t kIOAddrPCIConfigAddr = 0x0CF8;
  constexpr uint16_t kIOAddrPCIConfigData = 0x0CFC;
  WriteIOPort32(kIOAddrPCIConfigAddr,
                (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | reg);
  return ReadIOPort32(kIOAddrPCIConfigData);
}

bool PCI::DetectDevice(int bus, int device, int func) {
  constexpr uint32_t kPCIInvalidVendorID = 0xffff'ffff;
  uint32_t id = ReadPCIRegister(bus, device, func, 0);
  if (id == kPCIInvalidVendorID)
    return false;
  device_list_.insert({id,
                       {static_cast<uint8_t>(bus), static_cast<uint8_t>(device),
                        static_cast<uint8_t>(func)}});
  return true;
}

void PCI::DetectDevices() {
  for (int bus = 0x00; bus < 0xFF; bus++) {
    for (int device = 0x00; device < 32; device++) {
      if (!DetectDevice(bus, device, 0))
        continue;
      const uint32_t kPCIDeviceHasMultiFuncBit = (1 << 23);
      if ((ReadPCIRegister(bus, device, 0, 0xC) & kPCIDeviceHasMultiFuncBit) ==
          0)
        continue;
      for (uint32_t func = 1; func < 8; func++) {
        DetectDevice(bus, device, func);
      }
    }
  }
}

static uint16_t GetVendorID(uint32_t id) {
  return static_cast<uint16_t>(id);
}

static uint16_t GetDeviceID(uint32_t id) {
  return static_cast<uint16_t>(id >> 16);
}

void PCI::PrintDevices() {
  char s[128];
  for (auto& e : device_list_) {
    const uint16_t vendor_id = GetVendorID(e.first);
    const uint16_t device_id = GetDeviceID(e.first);
    snprintf(s, sizeof(s), "/%02X/%02X/%X %04X:%04X  %s\n", e.second.bus,
             e.second.device, e.second.func, vendor_id, device_id,
             GetDeviceName(e.first));
    PutString(s);
  }
}

const char* PCI::GetDeviceName(uint32_t key) {
  const auto& it = device_infos.find(key);
  return it != device_infos.end() ? it->second : "(Unknown)";
}
