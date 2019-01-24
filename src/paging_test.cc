#include <stdio.h>
#include <cassert>

#include "paging.h"

#ifdef LIUMOS_TEST

int kMaxPhyAddr = 36;

alignas(4096) IA_PML4 pml4;
alignas(4096) IA_PDPT pdpt;
alignas(4096) IA_PDT pdt;

void Test1GBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  constexpr uint64_t k1GBPageMapVirtBase = 0;
  constexpr uint64_t k1GBPageMapPhysBase = 1ULL << 30;
  assert(pml4.v2p(k1GBPageMapVirtBase) == kAddrCannotTranslate);
  pml4.SetPDPTForAddr(k1GBPageMapVirtBase, &pdpt, kPageAttrPresent);
  assert(pml4.v2p(k1GBPageMapVirtBase) == kAddrCannotTranslate);
  assert(pml4.GetPDPTForAddr(k1GBPageMapVirtBase) == &pdpt);
  pdpt.Set1GBPageForAddr(k1GBPageMapVirtBase, k1GBPageMapPhysBase,
                         kPageAttrPresent);
  assert(pml4.v2p(k1GBPageMapVirtBase) == k1GBPageMapPhysBase);
  pdpt.Set1GBPageForAddr(k1GBPageMapVirtBase, k1GBPageMapPhysBase, 0);
  assert(pml4.v2p(k1GBPageMapVirtBase) == kAddrCannotTranslate);
}
void Test2MBPageMapping() {
  pml4.ClearMapping();
  pdpt.ClearMapping();
  pdt.ClearMapping();
  constexpr uint64_t k2MBPageMapVirtBase = (2ULL << 21) + (1ULL << 30);
  constexpr uint64_t k2MBPageMapPhysBase = 1ULL << 21;
  assert(pml4.v2p(k2MBPageMapVirtBase) == kAddrCannotTranslate);
  pml4.SetPDPTForAddr(k2MBPageMapVirtBase, &pdpt, kPageAttrPresent);
  assert(pml4.v2p(k2MBPageMapVirtBase) == kAddrCannotTranslate);
  pdpt.SetPDTForAddr(k2MBPageMapVirtBase, &pdt, kPageAttrPresent);
  assert(pml4.v2p(k2MBPageMapVirtBase) == kAddrCannotTranslate);
  pdt.Set2MBPageForAddr(k2MBPageMapVirtBase, k2MBPageMapPhysBase,
                        kPageAttrPresent);
  assert(pml4.v2p(k2MBPageMapVirtBase) == k2MBPageMapPhysBase);
  pdt.Set2MBPageForAddr(k2MBPageMapVirtBase, k2MBPageMapPhysBase, 0);
  assert(pml4.v2p(k2MBPageMapVirtBase) == kAddrCannotTranslate);
}

int main() {
  Test1GBPageMapping();
  Test2MBPageMapping();
  puts("PASS");
  return 0;
}

#endif
