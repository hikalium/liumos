#include "liumos.h"

void PhysicalPageAllocator::Print() {
  FreeInfo* info = head_;
  while (info) {
    info->Print();
    info = info->GetNext();
  }
}

void PhysicalPageAllocator::FreePages(void* phys_addr, uint64_t num_of_pages) {
  // TODO: Impl merge
  assert(num_of_pages > 0);
  const uint64_t phys_addr_uint64 = reinterpret_cast<uint64_t>(phys_addr);
  assert((phys_addr_uint64 & 0xfff) == 0);

  FreeInfo* info = reinterpret_cast<FreeInfo*>(phys_addr);

  const uint32_t prox_domain =
      ACPI::srat ? ACPI::srat->GetProximityDomainForAddrRange(
                       phys_addr_uint64, num_of_pages << kPageSizeExponent)
                 : 0;
  head_ = new (info) FreeInfo(num_of_pages, head_, prox_domain);
}

void* PhysicalPageAllocator::FreeInfo::ProvidePages(uint64_t num_of_req_pages) {
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
  PutString(" )@ProxDomain:0x");
  PutHex64(proximity_domain_);
  PutString(" = 0x");
  PutHex64(num_of_pages_);
  PutString(" pages\n");
}
