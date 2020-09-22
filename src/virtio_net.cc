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
  const uint64_t buf_size = CalcSizeOfVirtqueue(queue_size);
  base_ = AllocMemoryForMappedIO<uint8_t*>(buf_size);
  bzero(base_, buf_size);
}

uint64_t Net::Virtqueue::GetPhysAddr() {
  return v2p(reinterpret_cast<uint64_t>(base_));
}

void Net::Virtqueue::SetDescriptor(int idx,
                                   void* vaddr,
                                   uint32_t len,
                                   uint16_t flags,
                                   uint16_t next) {
  assert(0 <= idx && idx < queue_size_);
  buf_[idx] = vaddr;
  Descriptor& desc =
      *reinterpret_cast<Descriptor*>(base_ + sizeof(Descriptor) * idx);
  desc.addr = v2p(vaddr);
  desc.len = len;
  desc.flags = flags;
  desc.next = next;
}

uint16_t Net::Virtqueue::GetUsedRingIndex() {
  // This function returns the index of used ring
  // which will be written by device on the next data arriving.
  volatile uint16_t& pidx = *reinterpret_cast<volatile uint16_t*>(
      base_ +
      CeilToPageAlignment(sizeof(Descriptor) * queue_size_ +
                          sizeof(uint16_t) * (2 * queue_size_)) +
      sizeof(uint16_t));
  return pidx;
}

Net::Virtqueue::UsedRingEntry& Net::Virtqueue::GetUsedRingEntry(int idx) {
  assert(0 <= idx && idx < queue_size_);
  UsedRingEntry* used_ring = reinterpret_cast<UsedRingEntry*>(
      base_ +
      CeilToPageAlignment(sizeof(Descriptor) * queue_size_ +
                          sizeof(uint16_t) * (2 * queue_size_)) +
      2 * sizeof(uint16_t));
  return used_ring[idx];
}

void PrintARPPacket(Net::ARPPacket& arp) {
  switch (arp.GetOperation()) {
    case Net::ARPPacket::Operation::kRequest:
      PutString("Who has ");
      arp.target_proto_addr.Print();
      PutString("? Tell ");
      arp.sender_proto_addr.Print();
      PutString(" at ");
      arp.sender_eth_addr.Print();
      PutChar('\n');
      return;
    case Net::ARPPacket::Operation::kReply:
      arp.sender_proto_addr.Print();
      PutString(" is at ");
      arp.sender_eth_addr.Print();
      PutChar('\n');
      return;
    default:
      break;
  }
  PutString("Recieved ARP with invalid Operation\n");
}

using InternetChecksum = Net::InternetChecksum;
using EtherFrame = Net::EtherFrame;
using ARPPacket = Net::ARPPacket;
using IPv4Packet = Net::IPv4Packet;
using ICMPPacket = Net::ICMPPacket;
using IPv4UDPPacket = Net::IPv4UDPPacket;

static bool ARPPacketHandler(uint8_t* frame_data, size_t frame_size) {
  if (frame_size < sizeof(ARPPacket)) {
    return false;
  }
  EtherFrame& eth = *reinterpret_cast<EtherFrame*>(frame_data);
  if (!eth.HasEthType(Net::EtherFrame::kTypeARP)) {
    return false;
  }
  ARPPacket& arp = *reinterpret_cast<ARPPacket*>(frame_data);
  Net& net = Net::GetInstance();
  if (arp.GetOperation() == ARPPacket::Operation::kReply) {
    Network::GetInstance().RegisterARPResolution(arp.sender_proto_addr,
                                                 arp.sender_eth_addr);
    PrintARPPacket(arp);
    return true;
  }
  if (arp.GetOperation() != ARPPacket::Operation::kRequest ||
      !arp.target_proto_addr.IsEqualTo(net.GetSelfIPv4Addr())) {
    // This is ARP, but not a request to me
    PrintARPPacket(arp);
    return true;
  }
  // Reply to ARP
  ARPPacket& reply = *net.GetNextTXPacketBuf<ARPPacket*>(sizeof(ARPPacket));
  reply.SetupReply(arp.sender_proto_addr, net.GetSelfIPv4Addr(),
                   arp.sender_eth_addr, net.GetSelfEtherAddr());
  net.SendPacket();
  return true;
}

static void SendICMPEchoReply(const ICMPPacket& req, size_t req_frame_size) {
  if (req_frame_size < sizeof(ICMPPacket)) {
    return;
  }
  PutStringAndHex("req_frame_size", req_frame_size);
  Net& net = Net::GetInstance();
  // Reply to ARP
  ICMPPacket& reply = *net.GetNextTXPacketBuf<ICMPPacket*>(req_frame_size);
  memcpy(&reply, &req, req_frame_size);
  // Setup ICMP
  reply.type = ICMPPacket::Type::kEchoReply;
  reply.csum.Clear();
  reply.csum = Network::InternetChecksum::Calc(
      &reply, offsetof(ICMPPacket, type), req_frame_size);
  // Setup IP
  reply.ip.dst_ip = req.ip.src_ip;
  reply.ip.src_ip = req.ip.dst_ip;
  reply.ip.csum.Clear();
  reply.ip.csum = Network::InternetChecksum::Calc(
      &reply, offsetof(IPv4Packet, version_and_ihl), req_frame_size);
  // Setup Eth
  reply.ip.eth.dst = req.ip.eth.src;
  reply.ip.eth.src = net.GetSelfEtherAddr();
  // Send
  net.SendPacket();
  PutString("Reply sent!: ");

  // UDP
  const char* s = "Hello! This is liumOS. Are you there?\n";
  uint16_t dst_port = 11111;
  uint16_t packet_size =
      static_cast<uint16_t>((sizeof(IPv4UDPPacket) + strlen(s) + 1) & ~1);
  IPv4UDPPacket& p = *net.GetNextTXPacketBuf<IPv4UDPPacket*>(packet_size);
  char* data = reinterpret_cast<char*>(reinterpret_cast<uint8_t*>(&p) +
                                       sizeof(IPv4UDPPacket));
  memcpy(&p, &req, packet_size);
  memcpy(data, s, strlen(s));
  // Setup UDP
  p.SetDestinationPort(dst_port);
  p.SetSourcePort(12345);
  p.SetDataSize(strlen(s));
  p.csum.Clear();
  p.csum = Network::CalcUDPChecksum(&p, offsetof(IPv4UDPPacket, src_port),
                                    packet_size, req.ip.dst_ip, req.ip.src_ip,
                                    p.length);
  // Setup IP
  p.ip.protocol = IPv4Packet::Protocol::kUDP;
  p.ip.SetDataLength(packet_size - sizeof(IPv4Packet));
  p.ip.dst_ip = req.ip.src_ip;
  p.ip.src_ip = req.ip.dst_ip;
  p.ip.csum.Clear();
  p.ip.csum = Network::InternetChecksum::Calc(
      &p, offsetof(IPv4Packet, version_and_ihl), sizeof(IPv4Packet));
  // Setup Eth
  p.ip.eth.dst = req.ip.eth.src;
  p.ip.eth.src = net.GetSelfEtherAddr();
  // Send
  net.SendPacket();
  PutString("UDP sent!: ");
}

static bool ICMPPacketHandler(IPv4Packet& p, size_t frame_size) {
  if (p.protocol != IPv4Packet::Protocol::kICMP) {
    return false;
  }
  if (frame_size < sizeof(ICMPPacket)) {
    return false;
  }
  PutString("ICMP: ");
  ICMPPacket& icmp = *reinterpret_cast<Net::ICMPPacket*>(&p);
  if (icmp.type == ICMPPacket::Type::kEchoRequest) {
    PutString("Echo Request\n");
    SendICMPEchoReply(icmp, frame_size);
  } else if (icmp.type == ICMPPacket::Type::kEchoReply) {
    PutString("Echo Reply\n");
  } else {
    PutString("Unknown type\n");
  }
  return true;
}

static bool UDPPacketHandler(IPv4Packet& p, size_t frame_size) {
  if (p.protocol != IPv4Packet::Protocol::kUDP) {
    return false;
  }
  if (frame_size < sizeof(IPv4UDPPacket)) {
    return false;
  }
  Net::IPv4UDPPacket& udp = *reinterpret_cast<Net::IPv4UDPPacket*>(&p);
  if (udp.GetDestinationPort() == 68) {
    PutString("DHCP: ");
    Net::DHCPPacket& dhcp = *reinterpret_cast<Net::DHCPPacket*>(&p);
    PutStringAndHex("op", dhcp.op);
    PutString("client MAC Addr: ");
    dhcp.chaddr.Print();
    PutString("\nassigned IP Addr: ");
    dhcp.yiaddr.Print();
    PutString("\n");
    Net& net = Net::GetInstance();
    if (dhcp.op == 2 && dhcp.chaddr.IsEqualTo(net.GetSelfEtherAddr())) {
      PutString("It's mine!!\n");
      net.SetSelfIPv4Addr(dhcp.yiaddr);
    }
  }
  return true;
}

static bool IPv4PacketHandler(uint8_t* frame_data, size_t frame_size) {
  using EtherFrame = Net::EtherFrame;
  using IPv4Packet = Net::IPv4Packet;
  if (frame_size < sizeof(IPv4Packet)) {
    return false;
  }
  EtherFrame& eth = *reinterpret_cast<Net::EtherFrame*>(frame_data);
  if (!eth.HasEthType(Net::EtherFrame::kTypeIPv4)) {
    return false;
  }
  IPv4Packet& p = *reinterpret_cast<Net::IPv4Packet*>(frame_data);
  if (ICMPPacketHandler(p, frame_size)) {
    return true;
  }
  if (UDPPacketHandler(p, frame_size)) {
    return true;
  }
  return true;
}

void Net::ProcessPacket(uint8_t* buf, size_t buf_size) {
  size_t frame_size = buf_size - sizeof(Net::PacketBufHeader);
  uint8_t* frame_data = buf + sizeof(Net::PacketBufHeader);
  ARPPacketHandler(frame_data, frame_size) ||
      IPv4PacketHandler(frame_data, frame_size);
  Network::GetInstance().PushToRXBuffer(frame_data, 0, frame_size);
}

void Net::PollRXQueue() {
  auto& rxq = vq_[kIndexOfRXVirtqueue];
  auto& rxq_cursor_ = vq_cursor_[kIndexOfRXVirtqueue];
  if (rxq.GetUsedRingIndex() == rxq_cursor_) {
    return;
  }
  while (rxq.GetUsedRingIndex() != rxq_cursor_) {
    int idx = rxq_cursor_ % vq_size_[kIndexOfRXVirtqueue];
    Net::ProcessPacket(rxq.GetDescriptorBuf(idx),
                       rxq.GetUsedRingEntry(idx).len);
    rxq.GetUsedRingEntry(idx).len = 0;
    rxq_cursor_++;
  }
  rxq.SetAvailableRingIndex(rxq_cursor_ - 1);
  WriteConfigReg16(16 /* Queue Notify */, kIndexOfRXVirtqueue);
}

void Net::SendPacket() {
  const int idx =
      vq_cursor_[kIndexOfTXVirtqueue] % vq_size_[kIndexOfTXVirtqueue];
  auto& txq = vq_[kIndexOfTXVirtqueue];
  uint8_t* data = txq.GetDescriptorBuf(idx);
  uint32_t data_size = txq.GetDescriptorSize(idx);
  kprintbuf("SendPacket data", data, sizeof(PacketBufHeader), data_size);
  PacketBufHeader& hdr = *txq.GetDescriptorBuf<PacketBufHeader*>(idx);
  hdr.flags = 0;
  hdr.gso_type = PacketBufHeader::kGSOTypeNone;
  hdr.header_length = 0x00;
  hdr.gso_size = 0;
  hdr.csum_start = 0;
  hdr.csum_offset = 0;
  txq.SetAvailableRingEntry(idx, idx);
  vq_cursor_[kIndexOfTXVirtqueue]++;
  txq.SetAvailableRingIndex(idx + 1);
  WriteConfigReg16(16 /* Queue Notify */, kIndexOfTXVirtqueue);
}

Net& Net::GetInstance() {
  if (!net_) {
    net_ = liumos->kernel_heap_allocator->Alloc<Net>();
    bzero(net_, sizeof(Net));
    new (net_) Net();
  }
  assert(net_);
  return *net_;
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
    vq_size_[i] = queue_size;
    vq_cursor_[i] = 0;
    PutStringAndHex("Queue Addr(phys)   ", vq_[i].GetPhysAddr());
    uint64_t vq_pfn = vq_[i].GetPhysAddr() >> kPageSizeExponent;
    assert(vq_pfn == (vq_pfn & 0xFFFF'FFFF));
    WriteConfigReg32(8, static_cast<uint32_t>(vq_pfn));
    PutStringAndHex("Queue Addr(RW)     ", ReadConfigReg32(8));
  }

  WriteDeviceStatus(ReadDeviceStatus() | kDeviceStatusDriverOK);

  PutString("MAC Addr: ");
  for (int i = 0; i < 6; i++) {
    mac_addr_.mac[i] = ReadConfigReg8(0x14 + i);
  }
  mac_addr_.Print();
  PutChar('\n');

  // Populate RX Buffer
  auto& rxq = vq_[kIndexOfRXVirtqueue];
  vq_cursor_[kIndexOfRXVirtqueue] = 0;
  for (int i = 0; i < vq_size_[kIndexOfRXVirtqueue]; i++) {
    rxq.SetDescriptor(i, AllocMemoryForMappedIO<void*>(kPageSize), kPageSize,
                      2 /* device write only */, 0);
    rxq.SetAvailableRingEntry(i, i);
    rxq.SetAvailableRingIndex(i + 1);
  }
  WriteConfigReg16(16 /* Queue Notify */, kIndexOfRXVirtqueue);

  // Populate TX Buffer
  auto& txq = vq_[kIndexOfTXVirtqueue];
  vq_cursor_[kIndexOfTXVirtqueue] = 0;
  for (int i = 0; i < vq_size_[kIndexOfTXVirtqueue]; i++) {
    txq.SetDescriptor(i, AllocMemoryForMappedIO<void*>(kPageSize), kPageSize,
                      0 /* device read only */, 0);
  }

  {
    // Send DHCP
    DHCPPacket& request = *GetNextTXPacketBuf<DHCPPacket*>(sizeof(DHCPPacket));
    request.SetupRequest(GetSelfEtherAddr());
    SendPacket();
  }
}
}  // namespace Virtio
