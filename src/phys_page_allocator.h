#pragma once
#include "generic.h"

class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_(nullptr) {}
  void FreePagesWithProximityDomain(void* phys_addr,
                                    uint64_t num_of_pages,
                                    uint32_t prox_domain) {
    // TODO: Impl merge
    assert(num_of_pages > 0);
    const uint64_t phys_addr_uint64 = reinterpret_cast<uint64_t>(phys_addr);
    assert((phys_addr_uint64 & 0xfff) == 0);
    FreeInfo* info = reinterpret_cast<FreeInfo*>(phys_addr);
    head_ = new (info) FreeInfo(num_of_pages, head_, prox_domain);
  }

  template <typename T>
  T AllocPages(uint64_t num_of_pages) {
    FreeInfo* info = head_;
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
    FreeInfo* info = head_;
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
  class FreeInfo {
   public:
    FreeInfo(uint64_t num_of_pages, FreeInfo* next, uint32_t proximity_domain)
        : num_of_pages_(num_of_pages),
          next_(next),
          proximity_domain_(proximity_domain) {}
    FreeInfo* GetNext() const { return next_; }
    void* ProvidePages(uint64_t num_of_req_pages) {
      if (!CanProvidePages(num_of_req_pages))
        return nullptr;
      num_of_pages_ -= num_of_req_pages;
      return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this) +
                                     (num_of_pages_ << 12));
    }

    void Print();
    uint32_t GetProximityDomain() { return proximity_domain_; };

   private:
    bool CanProvidePages(uint64_t num_of_req_pages) const {
      return num_of_req_pages < num_of_pages_;
    }

    uint64_t num_of_pages_;
    FreeInfo* next_;
    uint32_t proximity_domain_;
  };
  static_assert(sizeof(FreeInfo) <= kPageSize);

  FreeInfo* head_;
};
