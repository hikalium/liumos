#pragma once

#include <optional>

#include "generic.h"
#include "network.h"
#include "pci.h"

namespace Virtio {
class Net {
 public:
  struct PacketBufHeader {
    // virtio: 5.1.6 Device Operation
    uint8_t flags;
    uint8_t gso_type;
    uint16_t header_length;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
    //
    static constexpr uint8_t kFlagNeedsChecksum = 1;
    static constexpr uint8_t kGSOTypeNone = 0;
  };
  using InternetChecksum = Network::InternetChecksum;
  using EtherFrame = Network::EtherFrame;
  using IPv4Packet = Network::IPv4Packet;
  using ICMPPacket = Network::ICMPPacket;
  using DHCPPacket = Network::DHCPPacket;
  using IPv4UDPPacket = Network::IPv4UDPPacket;
  using ARPPacket = Network::ARPPacket;

  class Virtqueue {
   public:
    packed_struct Descriptor {
      volatile uint64_t addr;
      volatile uint32_t len;
      volatile uint16_t flags;
      volatile uint16_t next;
    };
    struct UsedRingEntry {
      volatile uint32_t id;   // index of start of used descriptor chain
      volatile uint32_t len;  // in byte
    };
    void Alloc(int queue_size);
    uint64_t GetPhysAddr();
    void SetDescriptor(int idx,
                       void* vaddr,
                       uint32_t len,
                       uint16_t flags,
                       uint16_t next);
    template <typename T = uint8_t*>
    T GetDescriptorBuf(int idx) {
      assert(0 <= idx && idx < queue_size_);
      return reinterpret_cast<T>(buf_[idx]);
    }
    uint32_t GetDescriptorSize(int idx) {
      assert(0 <= idx && idx < queue_size_);
      Descriptor& desc =
          *reinterpret_cast<Descriptor*>(base_ + sizeof(Descriptor) * idx);
      return desc.len;
    }
    void SetAvailableRingEntry(int idx, uint16_t desc_idx) {
      assert(0 <= idx && idx < queue_size_);
      volatile uint16_t* ring = reinterpret_cast<volatile uint16_t*>(
          base_ + sizeof(Descriptor) * queue_size_ + sizeof(uint16_t) * 2);
      ring[idx] = desc_idx;
    }
    void SetAvailableRingIndex(int idx) {
      volatile uint16_t& pidx = *reinterpret_cast<volatile uint16_t*>(
          base_ + sizeof(Descriptor) * queue_size_ + sizeof(uint16_t));
      pidx = idx;
    }
    uint16_t GetUsedRingIndex();
    UsedRingEntry& GetUsedRingEntry(int idx);

   private:
    static constexpr int kMaxQueueSize = 0x100;
    int queue_size_;
    uint8_t* base_;
    void* buf_[kMaxQueueSize];
  };

  void PollRXQueue();
  void Init();

  template <typename T = uint8_t*>
  T GetNextTXPacketBuf(size_t size) {
    uint32_t buf_size = static_cast<uint32_t>(sizeof(PacketBufHeader) + size);
    assert(buf_size < kPageSize);
    auto& txq = vq_[kIndexOfTXVirtqueue];
    const int idx =
        vq_cursor_[kIndexOfTXVirtqueue] % vq_size_[kIndexOfTXVirtqueue];
    txq.SetDescriptor(idx, txq.GetDescriptorBuf(idx), buf_size, 0, 0);
    return reinterpret_cast<T>(txq.GetDescriptorBuf(idx) +
                               sizeof(PacketBufHeader));
  }
  const Network::IPv4Addr GetSelfIPv4Addr() { return self_ip_; }
  void SetSelfIPv4Addr(Network::IPv4Addr addr) {
    self_ip_ = addr;
    if (self_ip_.IsEqualTo(Network::kWildcardIPv4Addr))
      return;
    Network::GetInstance().RegisterARPResolution(self_ip_, mac_addr_);
  }
  const Network::EtherAddr GetSelfEtherAddr() { return {mac_addr_}; }
  void SendPacket();

  static Net& GetInstance();

 private:
  static constexpr int kNumOfVirtqueues = 3;

  static constexpr int kIndexOfRXVirtqueue = 0;
  static constexpr int kIndexOfTXVirtqueue = 1;

  static Net* net_;
  PCI::DeviceLocation dev_;
  Network::EtherAddr mac_addr_;
  uint16_t config_io_addr_base_;
  Virtqueue vq_[kNumOfVirtqueues];
  uint16_t vq_size_[kNumOfVirtqueues];
  uint16_t vq_cursor_[kNumOfVirtqueues];
  Network::IPv4Addr self_ip_;
  bool debug_mode_enabled_;

  void ProcessPacket(uint8_t* buf, size_t buf_size);

  uint8_t ReadConfigReg8(int ofs);
  uint16_t ReadConfigReg16(int ofs);
  uint32_t ReadConfigReg32(int ofs);

  void WriteConfigReg8(int ofs, uint8_t data);
  void WriteConfigReg16(int ofs, uint16_t data);
  void WriteConfigReg32(int ofs, uint32_t data);

  uint8_t ReadDeviceStatus();
  void WriteDeviceStatus(uint8_t);
  void SetFeatures(uint32_t);
};
};  // namespace Virtio
