#include <stdio.h>
#include <cassert>

#include "paging.h"

#ifdef LIUMOS_TEST

int kMaxPhyAddr = 36;

alignas(4096) IA_PML4 pml4;
alignas(4096) IA_PDPT pdpt;
alignas(4096) IA_PDT pdt;
alignas(4096) IA_PT pt;

void Test1GBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  constexpr uint64_t k1GBPageVirtBase = 0;
  constexpr uint64_t k1GBPagePhysBase = 1ULL << 30;
  assert(pml4.v2p(k1GBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetPageBaseForAddr(k1GBPageVirtBase, &pdpt, kPageAttrPresent);
  assert(pml4.v2p(k1GBPageVirtBase) == kAddrCannotTranslate);
  assert(pml4.GetPageBaseForAddr(k1GBPageVirtBase) == &pdpt);
  pdpt.SetPageBaseForAddr(k1GBPageVirtBase, k1GBPagePhysBase, kPageAttrPresent);
  assert(pml4.v2p(k1GBPageVirtBase) == k1GBPagePhysBase);
  pdpt.SetPageBaseForAddr(k1GBPageVirtBase, k1GBPagePhysBase, 0);
  assert(pml4.v2p(k1GBPageVirtBase) == kAddrCannotTranslate);
}

void Test2MBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  constexpr uint64_t k2MBPageVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k2MBPagePhysBase = 1ULL << 21;
  assert(pml4.v2p(k2MBPageVirtBase) == kAddrCannotTranslate);
  pml4.SetPageBaseForAddr(k2MBPageVirtBase, &pdpt, kPageAttrPresent);
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
  pml4.SetPageBaseForAddr(k4KBPageVirtBase, &pdpt, kPageAttrPresent);
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

int main() {
  Test1GBPageMapping();
  Test2MBPageMapping();
  Test4KBPageMapping();
  puts("PASS");
  return 0;
}

#endif
