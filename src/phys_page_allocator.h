#pragma once
#include "generic.h"

class PhysicalPageAllocator {
 public:
  PhysicalPageAllocator() : head_(nullptr) {}
  void FreePages(void* phys_addr, uint64_t num_of_pages);
  template <typename T>
  T AllocPages(int num_of_pages) {
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
  void Print();

 private:
  class FreeInfo {
   public:
    FreeInfo(uint64_t num_of_pages, FreeInfo* next)
        : num_of_pages_(num_of_pages), next_(next) {}
    FreeInfo* GetNext() const { return next_; }
    void* ProvidePages(int num_of_req_pages);
    void Print();

   private:
    bool CanProvidePages(uint64_t num_of_req_pages) const {
      return num_of_req_pages < num_of_pages_;
    }

    uint64_t num_of_pages_;
    FreeInfo* next_;
  };
  static_assert(sizeof(FreeInfo) <= kPageSize);

  FreeInfo* head_;
};
