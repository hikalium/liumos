#include "virtio_net.h"

#include "kernel.h"

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

uint8_t Net::ReadConfigReg8(int ofs) {
  return ReadIOPort8(config_io_addr_base_ + ofs);
}
uint16_t Net::ReadConfigReg16(int ofs) {
  assert((ofs & 0b1) == 0);
  return ReadIOPort16(config_io_addr_base_ + ofs);
}
uint32_t Net::ReadConfigReg32(int ofs) {
  assert((ofs & 0b11) == 0);
  return ReadIOPort32(config_io_addr_base_ + ofs);
}

void Net::WriteConfigReg8(int ofs, uint8_t data) {
  WriteIOPort8(config_io_addr_base_ + ofs, data);
}
void Net::WriteConfigReg16(int ofs, uint16_t data) {
  assert((ofs & 0b1) == 0);
  WriteIOPort16(config_io_addr_base_ + ofs, data);
}
void Net::WriteConfigReg32(int ofs, uint32_t data) {
  assert((ofs & 0b11) == 0);
  WriteIOPort32(config_io_addr_base_ + ofs, data);
}

uint8_t Net::ReadDeviceStatus() {
  return ReadConfigReg8(18);
}
void Net::WriteDeviceStatus(uint8_t data) {
  WriteConfigReg8(18, data);
}
void Net::SetFeatures(uint32_t f) {
  WriteConfigReg32(4, f);
}

// 2.1 Device Status Field
constexpr static uint8_t kDeviceStatusAcknowledge = 1;
constexpr static uint8_t kDeviceStatusDriver = 2;
constexpr static uint8_t kDeviceStatusDriverOK = 4;
constexpr static uint8_t kDeviceStatusFeaturesOK = 8;
// constexpr static uint8_t kDeviceStatusDeviceNeedsReset = 64;
// constexpr static uint8_t kDeviceStatusFailed = 128;

constexpr static uint32_t kFeaturesMAC = (1 << 5);
constexpr static uint32_t kFeaturesStatus = (1 << 16);

static uint64_t CalcSizeOfVirtqueue(int queue_size) {
  // First part: Descriptor Table + Available Ring
  // Second part: Used Ring
  return CeilToPageAlignment(sizeof(Net::Virtqueue::Descriptor) * queue_size +
                             sizeof(uint16_t) * (2 + queue_size)) +
         CeilToPageAlignment(sizeof(uint16_t) * 2 +
                             sizeof(uint32_t) * 2 * queue_size);
}
void Net::Virtqueue::Alloc(int queue_size) {
  // 2.6.2 Legacy Interfaces: A Note on Virtqueue Layout
  assert(0 <= queue_size && queue_size <= kMaxQueueSize);
  queue_size_ = queue_size;
  base_ = AllocMemoryForMappedIO<uint8_t*>(CalcSizeOfVirtqueue(queue_size));
}

void Net::Init() {
  PutString("Virtio::Net::Init()\n");

  if (auto dev = FindVirtioNet()) {
    dev_ = *dev;
  } else {
    return;
  }
  PCI::EnsureBusMasterEnabled(dev_);
  PCI::BARForIO bar = PCI::GetBARForIO(dev_);
  config_io_addr_base_ = bar.base;
  PutStringAndHex("bar.base", bar.base);

  // PCI: 6.7. Capabilities List
  // 4.1.4 Virtio Structure PCI Capabilities
  uint8_t cap_ofs = static_cast<uint8_t>(PCI::ReadConfigRegister32(dev_, 0x34));
  for (; cap_ofs; cap_ofs = PCI::ReadConfigRegister8(dev_, cap_ofs + 1)) {
    uint8_t cap_id = PCI::ReadConfigRegister8(dev_, cap_ofs);
    PutStringAndHex("cap_ofs", cap_ofs);
    PutStringAndHex("id     ", cap_id);
    if (cap_id != 0x09 /* vendor-specific capability */)
      continue;
    uint8_t cap_type = PCI::ReadConfigRegister8(dev_, cap_ofs + 3);
    PutStringAndHex("type   ", cap_type);
    if (cap_type != 0x01)
      continue;
    uint8_t bar_idx = PCI::ReadConfigRegister8(dev_, cap_ofs + 4);
    PutStringAndHex("bar    ", bar_idx);
    uint32_t common_cfg_ofs = PCI::ReadConfigRegister32(dev_, cap_ofs + 8);
    PutStringAndHex("common_cfg_ofs ", common_cfg_ofs);
    uint32_t common_cfg_size = PCI::ReadConfigRegister32(dev_, cap_ofs + 12);
    PutStringAndHex("common_cfg_size", common_cfg_size);
  }

  // http://www.dumais.io/index.php?article=aca38a9a2b065b24dfa1dee728062a12
  // 3.1.1 Driver Requirements: Device Initialization
  WriteDeviceStatus(ReadDeviceStatus() | kDeviceStatusAcknowledge);
  WriteDeviceStatus(ReadDeviceStatus() | kDeviceStatusDriver);
  // 5.1.4.2 Driver Requirements: Device configuration layout
  // A driver SHOULD negotiate VIRTIO_NET_F_MAC if the device offers it
  SetFeatures(kFeaturesStatus | kFeaturesMAC);
  WriteDeviceStatus(ReadDeviceStatus() | kDeviceStatusFeaturesOK);

  // 5.1.5 Device Initialization
  // 4.1.5.1.3 Virtqueue Configuration
  for (int i = 0; i < kNumOfVirtqueues; i++) {
    WriteConfigReg16(14 /* queue_select */, i);
    uint16_t queue_size = ReadConfigReg16(12);
    if (!queue_size)
      break;
    PutStringAndHex("Queue Select(RW)   ", ReadConfigReg16(14));
    PutStringAndHex("Queue Size(R)      ", queue_size);
    vq_[i].Alloc(queue_size);
    PutStringAndHex("Queue Addr(phys)   ", vq_[i].GetPhysAddr());
    uint64_t vq_pfn = vq_[i].GetPhysAddr() >> kPageSizeExponent;
    assert(vq_pfn == (vq_pfn & 0xFFFF'FFFF));
    WriteConfigReg32(8, static_cast<uint32_t>(vq_pfn));
    PutStringAndHex("Queue Addr(RW)     ", ReadConfigReg32(8));
  }

  WriteDeviceStatus(ReadDeviceStatus() | kDeviceStatusDriverOK);

  PutString("MAC Addr: ");
  for (int i = 0; i < 6; i++) {
    mac_addr_[i] = ReadConfigReg8(0x14 + i);
    PutHex8ZeroFilled(mac_addr_[i]);
    PutChar(i == 5 ? '\n' : ':');
  }

  // Populate RX Buffer
  auto& rxq = vq_[kIndexOfRXVirtqueue];
  rxq.SetDescriptor(0, AllocMemoryForMappedIO<void*>(kPageSize), kPageSize,
                    2 /* device write only */, 0);
  rxq.SetAvailableRingIndex(1);
  WriteConfigReg16(16 /* Queue Notify */, kIndexOfRXVirtqueue);

  // Populate TX Buffer
  /*
     ARP: who has 10.10.10.135? Tell 10.10.10.90
     ff ff ff ff ff ff  // dst
     f8 ff c2 01 df 39  // src
     08 06              // ether type (ARP)
     00 01              // hardware type (ethernet)
     08 00              // protocol type (ipv4)
     06                 // hw addr len (6)
     04                 // proto addr len (4)
     00 01              // operation (arp request)
     f8 ff c2 01 df 39  // sender eth addr
     0a 0a 0a 5a        // sender protocol addr
     00 00 00 00 00 00  // target eth addr (0 since its unknown)
     0a 0a 0a 87        // target protocol addr
     */
  auto& txq = vq_[kIndexOfTXVirtqueue];
  txq.SetDescriptor(0, AllocMemoryForMappedIO<void*>(kPageSize), kPageSize, 0,
                    0);
  PacketBufHeader& hdr =
      *reinterpret_cast<PacketBufHeader*>(txq.GetDescriptorBuf(0));
  hdr.flags = PacketBufHeader::kFlagNeedsChecksum;
  hdr.gso_type = PacketBufHeader::kGSOTypeNone;
  hdr.header_length = sizeof(PacketBufHeader);
  hdr.gso_size = 0;
  hdr.csum_start = 0;
  hdr.csum_offset = sizeof(PacketBufHeader);
  hdr.num_buffers = 1;
  ARPPacket& arp = *reinterpret_cast<ARPPacket*>(
      reinterpret_cast<uint8_t*>(txq.GetDescriptorBuf(0)) +
      sizeof(PacketBufHeader));
  // As shown in https://wiki.qemu.org/Documentation/Networking
  uint8_t target_ip[4] = {10, 0, 2, 2};
  uint8_t src_ip[4] = {10, 0, 2, 15};
  arp.SetupRequest(target_ip, src_ip, mac_addr_);
  txq.SetDescriptor(0, txq.GetDescriptorBuf(0),
                    sizeof(PacketBufHeader) + sizeof(ARPPacket), 0, 0);
  txq.SetAvailableRingEntry(0, 0);
  txq.SetAvailableRingIndex(1);
  WriteConfigReg16(16 /* Queue Notify */, kIndexOfTXVirtqueue);

  PutStringAndHex("Device Features(R) ", ReadConfigReg32(0));
  PutStringAndHex("Device Features(RW)", ReadConfigReg32(4));
  PutStringAndHex("Device Status(RW)  ", ReadConfigReg8(18));
  PutStringAndHex("ISR Status(R)      ", ReadConfigReg8(19));

  uint8_t last_status = ReadConfigReg8(18);
  PutStringAndHex("Device Status(RW)  ", last_status);
  PutString("waiting for tx packet...");
  while (txq.GetUsedRingIndex() == 0) {
    uint8_t status = ReadConfigReg8(18);
    if (status == last_status)
      continue;
    PutStringAndHex("Device Status(RW)  ", status);
    last_status = status;
  }
  PutString("done");
  PutString("waiting for rx packet...");
  while (rxq.GetUsedRingIndex() == 0) {
    uint8_t status = ReadConfigReg8(18);
    if (status == last_status)
      continue;
    PutStringAndHex("Device Status(RW)  ", status);
    last_status = status;
  }
  PutString("done");
}

}  // namespace Virtio
