#include "phys_page_allocator.h"
#include "liumos.h"

template <class TStrategy>
void PhysicalPageAllocator<TStrategy>::FreeInfo::Print() {
  uint64_t physical_start = TStrategy::GetPhysAddrFromFreeInfo(this);
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
template void
PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>::FreeInfo::Print();

template <class TStrategy>
void PhysicalPageAllocator<TStrategy>::Print() {
  FreeInfo* info = TStrategy::GetFreeInfoFromPhysAddr(head_phys_addr_);
  while (info) {
    info->Print();
    info = info->GetNext();
  }
}
template void
PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>::Print();

PhysicalPageAllocator<UsePhysicalAddressInternallyStrategy>&
GetSystemDRAMAllocator() {
  assert(GetLoaderInfo().dram_allocator);
  return *GetLoaderInfo().dram_allocator;
}
