#pragma once
#include "generic.h"

struct UsePhysicalAddressInternallyStrategy;
struct UseKernelStraightMappingInternallyStrategy;

template <class TStrategy>
class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_phys_addr_(0) {}
  void FreePagesWithProximityDomain(uint64_t phys_addr,
                                    uint64_t num_of_pages,
                                    uint32_t prox_domain) {
    // TODO: Impl merge
    assert(num_of_pages > 0);
    assert((phys_addr & 0xfff) == 0);
    FreeInfo* info = TStrategy::GetFreeInfoFromPhysAddr(phys_addr);
    head_phys_addr_ = TStrategy::GetPhysAddrFromFreeInfo(new (info) FreeInfo(
        num_of_pages, TStrategy::GetFreeInfoFromPhysAddr(head_phys_addr_),
        prox_domain));
  }

  template <typename T>
  T AllocPages(uint64_t num_of_pages) {
    FreeInfo* info = TStrategy::GetFreeInfoFromPhysAddr(head_phys_addr_);
    void* addr = nullptr;
    while (info) {
      addr = info->ProvidePages(num_of_pages);
      if (addr)
        return reinterpret_cast<T>(addr);
      info = info->GetNext();
    }
    Panic("Cannot allocate pages");
  }
  template <typename T>
  T AllocPagesInProximityDomain(uint64_t num_of_pages,
                                uint32_t proximity_domain) {
    FreeInfo* info = TStrategy::GetFreeInfoFromPhysAddr(head_phys_addr_);
    void* addr = nullptr;
    while (info) {
      if (info->GetProximityDomain() == proximity_domain) {
        addr = info->ProvidePages(num_of_pages);
        if (addr)
          return reinterpret_cast<T>(addr);
      }
      info = info->GetNext();
    }
    Panic("Cannot allocate pages");
  }
  void Print();

 private:
  friend struct UsePhysicalAddressInternallyStrategy;
  friend struct UseKernelStraightMappingInternallyStrategy;

  class FreeInfo {
   public:
    FreeInfo(uint64_t num_of_pages, FreeInfo* next, uint32_t proximity_domain)
        : num_of_pages_(num_of_pages),
          next_phys_addr_(TStrategy::GetPhysAddrFromFreeInfo(next)),
          proximity_domain_(proximity_domain) {}
    FreeInfo* GetNext() const {
      return TStrategy::GetFreeInfoFromPhysAddr(next_phys_addr_);
    }
    void* ProvidePages(uint64_t num_of_req_pages) {
      if (!CanProvidePages(num_of_req_pages))
        return nullptr;
      num_of_pages_ -= num_of_req_pages;
      return reinterpret_cast<void*>(TStrategy::GetPhysAddrFromFreeInfo(this) +
                                     (num_of_pages_ << 12));
    }

    void Print();
    uint32_t GetProximityDomain() { return proximity_domain_; };

   private:
    bool CanProvidePages(uint64_t num_of_req_pages) const {
      return num_of_req_pages < num_of_pages_;
    }

    uint64_t num_of_pages_;
    uint64_t next_phys_addr_;
    uint32_t proximity_domain_;
  };
  static_assert(sizeof(FreeInfo) <= kPageSize);

  uint64_t head_phys_addr_;
};

PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>&
GetSystemDRAMAllocator();

struct UsePhysicalAddressInternallyStrategy {
  using FreeInfo =
      PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>::FreeInfo;
  static inline FreeInfo* GetFreeInfoFromPhysAddr(uint64_t paddr) {
    return reinterpret_cast<FreeInfo*>(paddr);
  }
  static inline uint64_t GetPhysAddrFromFreeInfo(FreeInfo* free_info) {
    return reinterpret_cast<uint64_t>(free_info);
  }
};

uint64_t GetKernelStraightMappingBase();
struct UseKernelStraightMappingInternallyStrategy {
  using FreeInfo = PhysicalPageAllocator<
      UseKernelStraightMappingInternallyStrategy>::FreeInfo;
  static inline FreeInfo* GetFreeInfoFromPhysAddr(uint64_t paddr) {
    return reinterpret_cast<FreeInfo*>(paddr + GetKernelStraightMappingBase());
  }
  static inline uint64_t GetPhysAddrFromFreeInfo(FreeInfo* free_info) {
    if (!free_info)
      return 0;
    return reinterpret_cast<uint64_t>(free_info) -
           GetKernelStraightMappingBase();
  }
};
using KernelPhysPageAllocator =
    PhysicalPageAllocator<UseKernelStraightMappingInternallyStrategy>;
