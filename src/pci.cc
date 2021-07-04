#include "pci.h"

#include <cstdio>
#include <string>

#include "kernel.h"
#include "liumos.h"

constexpr uint16_t kIOAddrPCIConfigAddr = 0x0CF8;
constexpr uint16_t kIOAddrPCIConfigData = 0x0CFC;

PCI* PCI::pci_;

static const std::
    unordered_multimap<PCI::DeviceIdent, const char*, PCI::DeviceIdentHash>
        device_infos = {
            {{0x1B36, 0x000D}, "QEMU XHCI Host Controller"},
            {{0x8086, 0x2918}, "82801IB (ICH9) LPC Interface Controller"},
            {{0x8086, 0x29c0}, "82G33/G31/P35/P31 Express DRAM Controller"},
            {{0x8086, 0x31A8}, "Intel XHCI Controller"},
            {{0x1234, 0x1111}, "QEMU Virtual Video Controller"},
            {{0x10ec, 0x8168},
             "RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller"},
            {{0x10ec, 0x8139}, "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter"},
            {{0x1af4, 0x1000}, "Virtio Network Card"},
};

static void SelectRegister(uint32_t bus,
                           uint32_t device,
                           uint32_t func,
                           uint32_t reg) {
  assert((bus & ~0b1111'1111) == 0);
  assert((device & ~0b1'1111) == 0);
  assert((func & ~0b111) == 0);
  assert((reg & ~0b1111'1100) == 0);
  WriteIOPort32(kIOAddrPCIConfigAddr,
                (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | reg);
}

uint8_t PCI::ReadConfigRegister8(uint32_t bus,
                                 uint32_t device,
                                 uint32_t func,
                                 uint32_t reg) {
  SelectRegister(bus, device, func, reg & 0b1111'1100);
  return (ReadIOPort32(kIOAddrPCIConfigData) >> (reg & 3)) & 0xFF;
}

uint32_t PCI::ReadConfigRegister32(uint32_t bus,
                                   uint32_t device,
                                   uint32_t func,
                                   uint32_t reg) {
  SelectRegister(bus, device, func, reg);
  return ReadIOPort32(kIOAddrPCIConfigData);
}

void PCI::WriteConfigRegister32(uint32_t bus,
                                uint32_t device,
                                uint32_t func,
                                uint32_t reg,
                                uint32_t value) {
  SelectRegister(bus, device, func, reg);
  WriteIOPort32(kIOAddrPCIConfigData, value);
}

bool PCI::DetectDevice(int bus, int device, int func) {
  constexpr uint32_t kPCIInvalidVendorID = 0xffff'ffff;
  uint32_t id = ReadConfigRegister32(bus, device, func, 0);
  if (id == kPCIInvalidVendorID)
    return false;
  device_list_.insert(
      {{static_cast<uint16_t>(id & 0xFFFF), static_cast<uint16_t>(id >> 16)},
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
      if ((ReadConfigRegister32(bus, device, 0, 0xC) &
           kPCIDeviceHasMultiFuncBit) == 0)
        continue;
      for (uint32_t func = 1; func < 8; func++) {
        DetectDevice(bus, device, func);
      }
    }
  }
}

void PCI::PrintDevices() {
  kprintf("device_infos len: %d\n", device_infos.size());
  char s[128];
  for (auto& e : device_list_) {
    const uint16_t vendor_id = e.first.vendor;
    const uint16_t device_id = e.first.device;
    const auto location = e.second;
    snprintf(s, sizeof(s), "/%02X/%02X/%X %04X:%04X  %s\n", location.bus,
             location.device, location.func, vendor_id, device_id,
             GetDeviceName(e.first));
    PutString(s);
    for (int i = 0; i < 6; i++) {
      uint32_t bar = ReadConfigRegister32(location, 0x10 + i * 4);
      // https://wiki.osdev.org/PCI
      if (bar == 0) {
        continue;
      }
      if (bar & 1) {
        // 1: I/O Space BAR
        snprintf(s, sizeof(s), "  BAR%d(I/O): 0x%08X\n", i, bar);
        PutString(s);
      } else {
        // 0: Memory Space BAR
        uint8_t type = (bar >> 1) & 3;
        snprintf(s, sizeof(s), "  BAR%d(Mem): 0x%08X type = %d\n", i, bar,
                 type);
        PutString(s);
      }
    }
  }
}

const char* PCI::GetDeviceName(DeviceIdent key) {
  const auto& it = device_infos.find(key);
  return it != device_infos.end() ? it->second : "(Unknown)";
}
