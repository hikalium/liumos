#pragma once
#include <unordered_map>

class PCI {
 public:
  struct DeviceLocation {
    uint8_t bus;
    uint8_t device;
    uint8_t func;
  };

  void DetectDevices();
  void PrintDevices();
  const auto& GetDeviceList() const { return device_list_; }

  static uint8_t ReadConfigRegister8(uint32_t bus,
                                     uint32_t device,
                                     uint32_t func,
                                     uint32_t reg);
  static uint8_t ReadConfigRegister8(const DeviceLocation& dev, uint32_t reg) {
    return ReadConfigRegister8(dev.bus, dev.device, dev.func, reg);
  }

  static uint32_t ReadConfigRegister32(uint32_t bus,
                                       uint32_t device,
                                       uint32_t func,
                                       uint32_t reg);
  static uint32_t ReadConfigRegister32(const DeviceLocation& dev,
                                       uint32_t reg) {
    return ReadConfigRegister32(dev.bus, dev.device, dev.func, reg);
  }
  static uint64_t ReadConfigRegister64(const DeviceLocation& dev,
                                       uint32_t reg) {
    return ReadConfigRegister32(dev, reg) |
           (static_cast<uint64_t>(ReadConfigRegister32(dev, reg + 4)) << 32);
  }
  static void WriteConfigRegister32(uint32_t bus,
                                    uint32_t device,
                                    uint32_t func,
                                    uint32_t reg,
                                    uint32_t value);
  static void WriteConfigRegister32(const DeviceLocation& dev,
                                    uint32_t reg,
                                    uint32_t value) {
    WriteConfigRegister32(dev.bus, dev.device, dev.func, reg, value);
  }
  static void WriteConfigRegister64(const DeviceLocation& dev,
                                    uint32_t reg,
                                    uint64_t value) {
    WriteConfigRegister32(dev, reg, static_cast<uint32_t>(value));
    WriteConfigRegister32(dev, reg + 4, static_cast<uint32_t>(value >> 32));
  }
  static const char* GetDeviceName(uint32_t key);
  static void EnsureBusMasterEnabled(DeviceLocation& dev) {
    constexpr uint32_t kPCIRegOffsetCommandAndStatus = 0x04;
    constexpr uint64_t kPCIRegCommandAndStatusMaskBusMasterEnable = 1 << 2;
    uint32_t cmd_and_status =
        ReadConfigRegister32(dev, kPCIRegOffsetCommandAndStatus);
    cmd_and_status |= (1 << 10);  // Interrupt Disable
    WriteConfigRegister32(dev, kPCIRegOffsetCommandAndStatus, cmd_and_status);
    assert(cmd_and_status & kPCIRegCommandAndStatusMaskBusMasterEnable);
  }
  struct BAR64 {
    uint64_t phys_addr;
    uint64_t size;
  };
  struct BARForIO {
    uint16_t base;
  };
  static BARForIO GetBARForIO(const DeviceLocation& dev) {
    constexpr uint32_t kPCIRegOffsetBAR = 0x10;
    constexpr uint64_t kPCIBARMaskType = 0b11;
    constexpr uint64_t kPCIBARBitsTypeIOSpace = 0b01;

    const uint64_t bar_raw_val =
        PCI::ReadConfigRegister32(dev, kPCIRegOffsetBAR);
    assert((bar_raw_val & kPCIBARMaskType) == kPCIBARBitsTypeIOSpace);
    return {static_cast<uint16_t>(bar_raw_val & ~kPCIBARMaskType)};
  }
  static BAR64 GetBAR64(const DeviceLocation& dev) {
    constexpr uint32_t kPCIRegOffsetBAR = 0x10;
    constexpr uint64_t kPCIBARMaskType = 0b111;
    constexpr uint64_t kPCIBARMaskAddr = ~0b1111ULL;
    constexpr uint64_t kPCIBARBitsType64bitMemorySpace = 0b100;

    const uint64_t bar_raw_val =
        PCI::ReadConfigRegister64(dev, kPCIRegOffsetBAR);
    PCI::WriteConfigRegister64(dev, kPCIRegOffsetBAR,
                               ~static_cast<uint64_t>(0));
    uint64_t base_addr_size_mask =
        PCI::ReadConfigRegister64(dev, kPCIRegOffsetBAR) & kPCIBARMaskAddr;
    uint64_t base_addr_size = ~base_addr_size_mask + 1;
    PCI::WriteConfigRegister64(dev, kPCIRegOffsetBAR, bar_raw_val);
    assert((bar_raw_val & kPCIBARMaskType) == kPCIBARBitsType64bitMemorySpace);
    const uint64_t base_addr = bar_raw_val & kPCIBARMaskAddr;
    return {base_addr, base_addr_size};
  }

  static PCI& GetInstance() {
    if (!pci_)
      pci_ = new PCI();
    assert(pci_);
    return *pci_;
  }

 private:
  PCI(){};
  bool DetectDevice(int bus, int device, int func);

  static PCI* pci_;
  std::unordered_multimap<uint32_t, DeviceLocation> device_list_;
};
