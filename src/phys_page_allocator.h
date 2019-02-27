#pragma once
#include "generic.h"

class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_(nullptr) {}
  void FreePages(void* phys_addr, uint64_t num_of_pages);
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
    void* ProvidePages(uint64_t num_of_req_pages);
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
