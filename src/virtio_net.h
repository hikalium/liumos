#pragma once

#include <optional>

#include "kernel.h"
#include "liumos.h"
#include "pci.h"

namespace Virtio {
class Net {
 public:
  class Virtqueue {
   public:
    struct Descriptor {
      volatile uint64_t addr;
      volatile uint32_t len;
      volatile uint16_t flags;
      volatile uint16_t next;
    };
    void Alloc(int queue_size);
    uint64_t GetPhysAddr() { return v2p(reinterpret_cast<uint64_t>(base_)); }
    void SetDescriptor(int idx,
                       uint64_t paddr,
                       uint32_t len,
                       uint16_t flags,
                       uint16_t next) {
      assert(0 <= idx && idx < queue_size_);
      Descriptor& desc =
          *reinterpret_cast<Descriptor*>(base_ + sizeof(Descriptor) * idx);
      desc.addr = paddr;
      desc.len = len;
      desc.flags = flags;
      desc.next = next;
    }
    void SetAvailableRingIndex(int idx) {
      volatile uint16_t& pidx = *reinterpret_cast<volatile uint16_t*>(
          base_ + sizeof(Descriptor) * queue_size_ + sizeof(uint16_t));
        pidx = idx;
    }
    uint16_t GetUsedRingIndex() {
      // This function returns the index of used ring
      // which will be written by device on the next data arriving.
      volatile uint16_t& pidx = *reinterpret_cast<volatile uint16_t*>(
          base_ + CeilToPageAlignment(sizeof(Descriptor) * queue_size_ + sizeof(uint16_t) * (2 * queue_size_)) + sizeof(uint16_t));
        return pidx;
    }

   private:
    int queue_size_;
    uint8_t* base_;
  };

  void Init();

  static Net& GetInstance() {
    if (!net_) {
      net_ = liumos->kernel_heap_allocator->Alloc<Net>();
      bzero(net_, sizeof(Net));
      new (net_) Net();
    }
    assert(net_);
    return *net_;
  }

 private:
  static constexpr int kNumOfVirtqueues = 3;

  static constexpr int kIndexOfRXVirtqueue = 0;
  static constexpr int kIndexOfTXVirtqueue = 1;

  static Net* net_;
  PCI::DeviceLocation dev_;
  uint8_t mac_addr_[6];
  uint16_t config_io_addr_base_;
  Virtqueue vq_[kNumOfVirtqueues];

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
