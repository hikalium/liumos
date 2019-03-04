#include <stdio.h>
#include <stdlib.h>
#include <cassert>

[[noreturn]] void Panic(const char* s) {
  puts(s);
  exit(EXIT_FAILURE);
}

#include "paging.h"

int kMaxPhyAddr = 36;

alignas(4096) IA_PML4 pml4;
alignas(4096) IA_PDPT pdpt;
alignas(4096) IA_PDT pdt;
alignas(4096) IA_PT pt;

PhysicalPageAllocator dummy_allocator;

void Test1GBPageMapping(const uint64_t virt_base, const uint64_t phys_base) {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  assert(v2p(pml4, virt_base) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(virt_base, &pdpt, kPageAttrPresent);
  assert(pml4.GetTableBaseForAddr(virt_base) == &pdpt);
  assert(v2p(pml4, virt_base) == kAddrCannotTranslate);
  pdpt.SetPageBaseForAddr(virt_base, phys_base, kPageAttrPresent);
  assert(v2p(pml4, virt_base) == phys_base);
  pdpt.SetPageBaseForAddr(virt_base, phys_base, 0);
  assert(v2p(pml4, virt_base) == kAddrCannotTranslate);
}

void Test2MBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  constexpr uint64_t k2MBPageVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k2MBPagePhysBase = 1ULL << 21;
  assert(v2p(pml4, k2MBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(k2MBPageVirtBase, &pdpt, kPageAttrPresent);
  assert(v2p(pml4, k2MBPageVirtBase) == kAddrCannotTranslate);
  pdpt.SetTableBaseForAddr(k2MBPageVirtBase, &pdt, kPageAttrPresent);
  assert(v2p(pml4, k2MBPageVirtBase) == kAddrCannotTranslate);
  pdt.SetPageBaseForAddr(k2MBPageVirtBase, k2MBPagePhysBase, kPageAttrPresent);
  assert(v2p(pml4, k2MBPageVirtBase) == k2MBPagePhysBase);
  pdt.SetPageBaseForAddr(k2MBPageVirtBase, k2MBPagePhysBase, 0);
  assert(v2p(pml4, k2MBPageVirtBase) == kAddrCannotTranslate);
}

void Test4KBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  pt.ClearMapping();
  constexpr uint64_t k4KBPageVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k4KBPagePhysBase = 1ULL << 21;
  assert(v2p(pml4, k4KBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(k4KBPageVirtBase, &pdpt, kPageAttrPresent);
  assert(v2p(pml4, k4KBPageVirtBase) == kAddrCannotTranslate);
  pdpt.SetTableBaseForAddr(k4KBPageVirtBase, &pdt, kPageAttrPresent);
  assert(v2p(pml4, k4KBPageVirtBase) == kAddrCannotTranslate);
  pdt.SetTableBaseForAddr(k4KBPageVirtBase, &pt, kPageAttrPresent);
  assert(v2p(pml4, k4KBPageVirtBase) == kAddrCannotTranslate);
  pt.SetPageBaseForAddr(k4KBPageVirtBase, k4KBPagePhysBase, kPageAttrPresent);
  assert(v2p(pml4, k4KBPageVirtBase) == k4KBPagePhysBase);
  pt.SetPageBaseForAddr(k4KBPageVirtBase, k4KBPagePhysBase, 0);
  assert(v2p(pml4, k4KBPageVirtBase) == kAddrCannotTranslate);
}

void TestRangeMapping(IA_PML4& pml4,
                      uint64_t vaddr,
                      uint64_t paddr,
                      uint64_t size) {
  pml4.ClearMapping();
  constexpr int kPageTableBufferSize = 1024;
  uint64_t malloc_addr = reinterpret_cast<uint64_t>(
      malloc(kPageSize * (kPageTableBufferSize + 1)));
  if (!malloc_addr) {
    perror("malloc failed.\n");
    exit(EXIT_FAILURE);
  }
  dummy_allocator.FreePagesWithProximityDomain(
      reinterpret_cast<void*>((malloc_addr + kPageSize - 1) & ~kPageAddrMask),
      kPageTableBufferSize, 0);
  CreatePageMapping(dummy_allocator, pml4, vaddr, paddr, size,
                    kPageAttrPresent);
  assert(v2p(pml4, vaddr - 1) == kAddrCannotTranslate);
  assert(v2p(pml4, vaddr) == paddr);
  assert(v2p(pml4, vaddr + size - 1) == paddr + size - 1);
  assert(v2p(pml4, vaddr + size) == kAddrCannotTranslate);
}

int main() {
  Test1GBPageMapping(0, 1ULL << 30);
  Test1GBPageMapping(1ULL << 30, 1ULL << 31);
  Test2MBPageMapping();
  Test4KBPageMapping();
  TestRangeMapping(pml4, 0x0000'0000'0000'3000, 0x0000'0000'1000'7000,
                   1ULL * 1024 * 1024 * 1024);
  puts("PASS");
  return 0;
}
