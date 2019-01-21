#include "liumos.h"

IA_PT* IA_PDE::GetPTAddr() {
  return reinterpret_cast<IA_PT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                  ~((1ULL << 12) - 1));
}

void IA_PDT::Print() {
  for (int i = 0; i < kNumOfPDE; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("PDT[");
    PutHex64(i);
    PutString("]:\n");
    if (entries[i].Is2MBPage()) {
      PutString(" 2MB Page\n");
      continue;
    }
    PutStringAndHex(" addr", entries[i].GetPTAddr());
  }
}

IA_PDT* IA_PDPTE::GetPDTAddr() {
  return reinterpret_cast<IA_PDT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                   ~((1ULL << 12) - 1));
}

void IA_PDPT::Print() {
  for (int i = 0; i < kNumOfPDPTE; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("PDPT[");
    PutHex64(i);
    PutString("]:\n");
    if (entries[i].Is1GBPage()) {
      PutString(" 1GB Page\n");
      continue;
    }
    IA_PDT* pdt = entries[i].GetPDTAddr();
    pdt->Print();
  }
}

IA_PDPT* IA_PML4E::GetPDPTAddr() {
  return reinterpret_cast<IA_PDPT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                    ~((1ULL << 12) - 1));
}

void IA_PML4::Print() {
  for (int i = 0; i < kNumOfPML4E; i++) {
    if (!entries[i].IsPresent())
      continue;
    PutString("PML4[");
    PutHex64(i);
    PutString("]:\n");
    IA_PDPT* pdpt = entries[i].GetPDPTAddr();
    pdpt->Print();
  }
}
