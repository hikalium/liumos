#include <unordered_map>

class PCI {
 public:
  void DetectDevices();
  void PrintDevices();

  static PCI* GetInstance() {
    if (!pci_)
      pci_ = new PCI();
    return pci_;
  }

 private:
  PCI(){};
  struct DeviceLocation {
    uint8_t bus;
    uint8_t device;
    uint8_t func;
  };

  static PCI* pci_;
  std::unordered_multimap<uint32_t, DeviceLocation> device_list_;
};
