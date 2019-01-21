#include <stdio.h>
#include <cassert>

#include "paging.h"

#ifdef LIUMOS_TEST

int kMaxPhyAddr = 36;

alignas(4096) IA_PML4 pml4;
alignas(4096) IA_PDPT pdpt1;

int main() {
  assert(pml4.v2p(0) == kAddrCannotTranslate);
  pml4.SetPDPTForAddr(0, &pdpt1, kPageAttrPresent);
  assert(pml4.v2p(0) == kAddrCannotTranslate);
  assert(pml4.GetPDPTForAddr(0) == &pdpt1);
  pdpt1.Set1GBPageForAddr(0, 1ULL << 30, kPageAttrPresent);
  assert(pml4.v2p(0) == (1ULL << 30));
  pdpt1.Set1GBPageForAddr(0, 1ULL << 30, 0);
  assert(pml4.v2p(0) == kAddrCannotTranslate);

  puts("PASS");
  return 0;
}

#endif
