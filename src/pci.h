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

  static uint32_t ReadConfigRegister32(uint32_t bus,
                                       uint32_t device,
                                       uint32_t func,
                                       uint32_t reg);
  static uint32_t ReadConfigRegister32(DeviceLocation& dev, uint32_t reg) {
    return ReadConfigRegister32(dev.bus, dev.device, dev.func, reg);
  }
  static uint64_t ReadConfigRegister64(DeviceLocation& dev, uint32_t reg) {
    return ReadConfigRegister32(dev, reg) |
           (static_cast<uint64_t>(ReadConfigRegister32(dev, reg + 4)) << 32);
  }
  static void WriteConfigRegister32(uint32_t bus,
                                    uint32_t device,
                                    uint32_t func,
                                    uint32_t reg,
                                    uint32_t value);
  static void WriteConfigRegister32(DeviceLocation& dev,
                                    uint32_t reg,
                                    uint32_t value) {
    WriteConfigRegister32(dev.bus, dev.device, dev.func, reg, value);
  }
  static void WriteConfigRegister64(DeviceLocation& dev,
                                    uint32_t reg,
                                    uint64_t value) {
    WriteConfigRegister32(dev, reg, static_cast<uint32_t>(value));
    WriteConfigRegister32(dev, reg + 4, static_cast<uint32_t>(value >> 32));
  }
  static const char* GetDeviceName(uint32_t key);

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
