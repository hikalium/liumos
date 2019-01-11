#include "liumos.h"

void PhysicalPageAllocator::Print() {
  FreeInfo* info = head_;
  PutString("Free lists:\n");
  while (info) {
    info->Print();
    info = info->GetNext();
  }
  PutString("Free lists end\n");
}

void PhysicalPageAllocator::FreePages(void* phys_addr, uint64_t num_of_pages) {
  // TODO: Impl merge
  assert(phys_addr != nullptr);
  assert(num_of_pages > 0);
  assert((reinterpret_cast<uint64_t>(phys_addr) & 0xfff) == 0);

  FreeInfo* info = reinterpret_cast<FreeInfo*>(phys_addr);
  head_ = new (info) FreeInfo(num_of_pages, head_);
}

void* PhysicalPageAllocator::AllocPages(int num_of_pages) {
  FreeInfo* info = head_;
  void* addr = nullptr;
  while (info) {
    addr = info->ProvidePages(num_of_pages);
    if (addr)
      return addr;
  }
  Panic("Cannot allocate pages");
}

void* PhysicalPageAllocator::FreeInfo::ProvidePages(int num_of_req_pages) {
  if (!CanProvidePages(num_of_req_pages))
    return nullptr;
  num_of_pages_ -= num_of_req_pages;
  return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this) +
                                 (num_of_pages_ << 12));
}

void PhysicalPageAllocator::FreeInfo::Print() {
  uint64_t physical_start = reinterpret_cast<uint64_t>(this);
  PutString("[ 0x");
  PutHex64ZeroFilled(physical_start);
  PutString(" - 0x");
  PutHex64ZeroFilled(physical_start + (num_of_pages_ << 12));
  PutString(" ) = 0x");
  PutHex64(num_of_pages_);
  PutString(" pages\n");
}
