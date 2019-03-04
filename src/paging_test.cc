#include <stdio.h>
#include <stdlib.h>
#include <cassert>

void Panic(const char* s) {
  puts(s);
  exit(EXIT_FAILURE);
}

#include "paging.h"

int kMaxPhyAddr = 36;

alignas(4096) IA_PML4 pml4;
alignas(4096) IA_PDPT pdpt;
alignas(4096) IA_PDT pdt;
alignas(4096) IA_PT pt;

void Test1GBPageMapping(const uint64_t virt_base, const uint64_t phys_base) {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  assert(pml4.v2p(virt_base) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(virt_base, &pdpt, kPageAttrPresent);
  assert(pml4.GetTableBaseForAddr(virt_base) == &pdpt);
  assert(pml4.v2p(virt_base) == kAddrCannotTranslate);
  pdpt.SetPageBaseForAddr(virt_base, phys_base, kPageAttrPresent);
  assert(pml4.v2p(virt_base) == phys_base);
  pdpt.SetPageBaseForAddr(virt_base, phys_base, 0);
  assert(pml4.v2p(virt_base) == kAddrCannotTranslate);
}

void Test2MBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  constexpr uint64_t k2MBPageVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k2MBPagePhysBase = 1ULL << 21;
  assert(pml4.v2p(k2MBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(k2MBPageVirtBase, &pdpt, kPageAttrPresent);
  assert(pml4.v2p(k2MBPageVirtBase) == kAddrCannotTranslate);
  pdpt.SetTableBaseForAddr(k2MBPageVirtBase, &pdt, kPageAttrPresent);
  assert(pml4.v2p(k2MBPageVirtBase) == kAddrCannotTranslate);
  pdt.SetPageBaseForAddr(k2MBPageVirtBase, k2MBPagePhysBase, kPageAttrPresent);
  assert(pml4.v2p(k2MBPageVirtBase) == k2MBPagePhysBase);
  pdt.SetPageBaseForAddr(k2MBPageVirtBase, k2MBPagePhysBase, 0);
  assert(pml4.v2p(k2MBPageVirtBase) == kAddrCannotTranslate);
}

void Test4KBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  pt.ClearMapping();
  constexpr uint64_t k4KBPageVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k4KBPagePhysBase = 1ULL << 21;
  assert(pml4.v2p(k4KBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetTableBaseForAddr(k4KBPageVirtBase, &pdpt, kPageAttrPresent);
  assert(pml4.v2p(k4KBPageVirtBase) == kAddrCannotTranslate);
  pdpt.SetTableBaseForAddr(k4KBPageVirtBase, &pdt, kPageAttrPresent);
  assert(pml4.v2p(k4KBPageVirtBase) == kAddrCannotTranslate);
  pdt.SetTableBaseForAddr(k4KBPageVirtBase, &pt, kPageAttrPresent);
  assert(pml4.v2p(k4KBPageVirtBase) == kAddrCannotTranslate);
  pt.SetPageBaseForAddr(k4KBPageVirtBase, k4KBPagePhysBase, kPageAttrPresent);
  assert(pml4.v2p(k4KBPageVirtBase) == k4KBPagePhysBase);
  pt.SetPageBaseForAddr(k4KBPageVirtBase, k4KBPagePhysBase, 0);
  assert(pml4.v2p(k4KBPageVirtBase) == kAddrCannotTranslate);
}

void TestRangeMapping(IA_PML4& pml4,
                      uint64_t paddr,
                      uint64_t vaddr,
                      uint64_t size) {}

int main() {
  Test1GBPageMapping(0, 1ULL << 30);
  Test1GBPageMapping(1ULL << 30, 1ULL << 31);
  Test2MBPageMapping();
  Test4KBPageMapping();
  puts("PASS");
  return 0;
}
