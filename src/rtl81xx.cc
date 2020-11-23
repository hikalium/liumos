#include "rtl81xx.h"
#include "kernel.h"
#include "network.h"

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
    if (!it.first.HasID(0x10EC, 0x8168)) {
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

uint16_t RTL81::ReadPHYReg(uint8_t addr) {
  WriteIOPort32(io_addr_base_ + 0x60, addr << 16);
  while (true) {
    uint32_t v = ReadIOPort32(io_addr_base_ + 0x60);
    if (v >> 31)
      return v;
  }
}

void RTL81::Init() {
  // https://wiki.osdev.org/RTL8139
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

  Network::EtherAddr eth_addr;
  for (int i = 0; i < 6; i++) {
    eth_addr.mac[i] = ReadIOPort8(io_addr_base_ + i);
  }
  kprintf("MAC Addr: ");
  eth_addr.Print();
  kprintf("\n");

  kprintf("Resetting the controller...");
  // https://wiki.osdev.org/RTL8169
  WriteIOPort8(io_addr_base_ + 0x37, 0x10);
  /*set the Reset bit (0x10) to the Command Register (0x37)*/
  while (ReadIOPort8(io_addr_base_ + 0x37) & 0x10) {
    kprintf(".");
    asm volatile("pause");
  }
  kprintf(" done.\n");

  uint32_t transmit_config = ReadIOPort32(io_addr_base_ + 0x40);
  kprintf("Transmit Config: 0x%08X\n", transmit_config);
  uint32_t receive_config = ReadIOPort32(io_addr_base_ + 0x44);
  kprintf("Receive Config: 0x%08X\n", receive_config);

  // Setup recv buffer
  rx_descriptors_ = AllocMemoryForMappedIO<RXCommandDescriptor*>(
      kNumOfRXDescriptors * sizeof(RXCommandDescriptor));
  for (int i = 0; i < kNumOfRXDescriptors; i++) {
    rx_buffers_[i] = AllocMemoryForMappedIO<void*>(kSizeOfEachRXBuffer);
    rx_descriptors_[i].buf_size_and_flag =
        kSizeOfEachRXBuffer | kRXFlagsOwnedByController;
    rx_descriptors_[i].vlan_info = 0;
    rx_descriptors_[i].buf_phys_addr = v2p(rx_buffers_[i]);
  }
  // Mark EOR
  rx_descriptors_[kNumOfRXDescriptors - 1].buf_size_and_flag |=
      kRXFlagsEndOfRing;
  uint64_t rx_desc_phys_addr = v2p(rx_descriptors_);
  kprintf("rx_descriptors_: %p\n", rx_descriptors_);
  kprintf("rx_desc_phys_addr: 0x%016llX\n", rx_desc_phys_addr);
  // Set RDSAR
  WriteIOPort32(io_addr_base_ + 0xE4, static_cast<uint32_t>(rx_desc_phys_addr));
  WriteIOPort32(io_addr_base_ + 0xE8,
                static_cast<uint32_t>(rx_desc_phys_addr >> 32));
  kprintf("RDSAR: %016llX\n",
          (static_cast<uint64_t>(ReadIOPort32(io_addr_base_ + 0xE8)) << 32) |
              ReadIOPort32(io_addr_base_ + 0xE4));
  // Set Receive Packet Maximum Size (RMS)
  WriteIOPort16(io_addr_base_ + 0xDA, kSizeOfEachRXBuffer);
  // Set Receive Config to receive all packets
  WriteIOPort32(io_addr_base_ + 0x44, 0b1000'1111'0001'1111);
  // Start receiver
  WriteIOPort8(io_addr_base_ + 0x37, 1 << 3);

  bool last_is_link_up = false;
  uint32_t last_rx_desc_flags = 0;
  uint16_t last_isr = 0;
  uint16_t last_rxconf = 0;
  uint8_t last_cmd = 0;
  int idx = 0;
  while (true) {
    uint8_t phy_status = ReadIOPort8(io_addr_base_ + 0x6C);
    bool is_link_up = phy_status & 2;
    uint32_t rx_desc_flags = rx_descriptors_[idx].buf_size_and_flag;
    uint16_t isr = ReadIOPort16(io_addr_base_ + 0x3E);
    uint16_t rxconf = ReadIOPort16(io_addr_base_ + 0x44);
    uint8_t cmd = ReadIOPort8(io_addr_base_ + 0x37);
    if (last_is_link_up != is_link_up || last_rx_desc_flags != rx_desc_flags ||
        last_isr != isr || last_rxconf != rxconf || last_cmd != cmd) {
      kprintf("Link: %s, 0x%08X, isr: 0x%04X, rxconf: 0x%08X, Cmd: 0x%02X\n",
              is_link_up ? "UP" : "DOWN", rx_desc_flags, isr, rxconf, cmd);
      last_is_link_up = is_link_up;
      last_rx_desc_flags = rx_desc_flags;
      last_isr = isr;
      last_rxconf = rxconf;
      last_cmd = cmd;
    }
    if ((rx_desc_flags & kRXFlagsOwnedByController) == 0) {
      size_t size = rx_descriptors_[idx].buf_size_and_flag & 0x1FFF;
      kprintf("update on rx_descriptors_[%d]: recieved size = %d\n", idx, size);
      // volatile void *data = rx_buffers_[idx];
      // kprintbuf("received", data, 0, size);

      if (rx_descriptors_[idx].buf_size_and_flag & kRXFlagsEndOfRing) {
        rx_descriptors_[idx].vlan_info = 0;
        rx_descriptors_[idx].buf_size_and_flag =
            kSizeOfEachRXBuffer | kRXFlagsEndOfRing | kRXFlagsOwnedByController;
        idx = 0;
        last_rx_desc_flags = 0;
      } else {
        rx_descriptors_[idx].vlan_info = 0;
        rx_descriptors_[idx].buf_size_and_flag =
            kSizeOfEachRXBuffer | kRXFlagsOwnedByController;
        idx++;
        last_rx_desc_flags = 0;
      }
    }
    if ((cmd & 0x08) == 0) {
      // Receiver has been stopped
      break;
    }
    if (liumos->main_console->GetCharWithoutBlocking() != KeyID::kNoInput)
      break;
  }
}
