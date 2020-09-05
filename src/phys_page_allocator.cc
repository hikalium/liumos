#include "liumos.h"

void PhysicalPageAllocator::Print() {
  FreeInfo* info = head_;
  while (info) {
    info->Print();
    info = info->GetNext();
  }
}

void PhysicalPageAllocator::FreeInfo::Print() {
  uint64_t physical_start = reinterpret_cast<uint64_t>(this);
  PutString("[ 0x");
  PutHex64ZeroFilled(physical_start);
  PutString(" - 0x");
  PutHex64ZeroFilled(physical_start + (num_of_pages_ << 12));
  PutString(" )@ProxDomain:0x");
  PutHex64(proximity_domain_);
  PutString(" = 0x");
  PutHex64(num_of_pages_);
  PutString(" pages\n");
}

PhysicalPageAllocator& GetSystemDRAMAllocator() {
  assert(GetLoaderInfo().dram_allocator);
  return *GetLoaderInfo().dram_allocator;
}
