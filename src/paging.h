#pragma once

#include "generic.h"
#include "phys_page_allocator.h"
#include "sys_constant.h"

constexpr uint64_t kAddrCannotTranslate =
    0x8000'0000'0000'0000;  // non-canonical address

constexpr uint64_t kPageAttrMask = 0b11111;
constexpr uint64_t kPageAttrPresent = 0b00001;
constexpr uint64_t kPageAttrWritable = 0b00010;
constexpr uint64_t kPageAttrUser = 0b00100;
constexpr uint64_t kPageAttrWriteThrough = 0b01000;
constexpr uint64_t kPageAttrCacheDisable = 0b10000;

static inline uint64_t CeilToPageAlignment(uint64_t v) {
  return (v + kPageSize - 1) & ~kPageAddrMask;
}
static inline uint64_t FloorToPageAlignment(uint64_t v) {
  return v & ~kPageAddrMask;
}
static inline bool IsAlignedToPageSize(uint64_t v) {
  return (v & kPageAddrMask) == 0;
}
static inline bool IsAlignedToPageSize(const void* a) {
  return IsAlignedToPageSize(reinterpret_cast<uint64_t>(a));
}

template <class T, class V>
using IsValidType = V;

template <class S, class V>
using CanHaveTable = IsValidType<typename S::NextTableType, V>;
template <class S>
constexpr auto CanHaveTableF_(int) -> CanHaveTable<S, int> {
  return 1;
};
template <class S>
constexpr int CanHaveTableF_(...) {
  return 0;
};
template <class S>
constexpr int CanHaveTableF() {
  return CanHaveTableF_<S>(0);
};
template <class S>
constexpr int CannotHaveTableF() {
  return !CanHaveTableF<S>();
};
template <class S>
constexpr void CannotHaveTable_(char (*)[CannotHaveTableF<S>()] = 0) {}
template <class S, class V>
using CannotHaveTable = IsValidType<decltype(CannotHaveTable_<S>()), V>;

template <class S, class V>
using CanHavePage = IsValidType<decltype(S::kBitsOfOffsetInPage), V>;
template <class S>
constexpr auto CanHavePageF_(int) -> CanHavePage<S, int> {
  return 1;
};
template <class S>
constexpr int CanHavePageF_(...) {
  return 0;
};
template <class S>
constexpr int CanHavePageF() {
  return CanHavePageF_<S>(0);
};
template <class S>
constexpr int CannotHavePageF() {
  return !CanHavePageF<S>();
};
template <class S>
constexpr void CannotHavePage_(char (*)[CannotHavePageF<S>()] = 0) {}
template <class S, class V>
using CannotHavePage = IsValidType<decltype(CannotHavePage_<S>()), V>;

template <typename TEntryType>
struct PageTableStruct {
  using EntryType = TEntryType;
  static constexpr int kNumOfEntries = (1 << 9);
  static constexpr int kIndexMask = kNumOfEntries - 1;
  static constexpr int kIndexShift = TEntryType::kIndexShift;
  static constexpr uint64_t kOffsetMask = (1ULL << kIndexShift) - 1;
  static inline int addr2index(uint64_t addr) {
    return (addr >> kIndexShift) & kIndexMask;
  }
  TEntryType entries[kNumOfEntries];
  TEntryType& GetEntryForAddr(uint64_t vaddr) {
    return entries[addr2index(vaddr)];
  }
  void ClearMapping() {
    for (int i = 0; i < kNumOfEntries; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  template <class S = typename EntryType::Strategy>
  typename S::NextTableType* GetTableBaseForAddr(uint64_t vaddr) {
    return GetEntryForAddr(vaddr).GetTableAddr();
  }
  template <class S = typename EntryType::Strategy>
  typename S::NextTableType* SetTableBaseForAddr(
      uint64_t addr,
      typename S::NextTableType* new_ent,
      uint64_t attr) {
    EntryType& e = GetEntryForAddr(addr);
    typename S::NextTableType* old_e = e.GetTableAddr();
    e.SetTableAddr(new_ent, attr);
    return old_e;
  }
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    GetEntryForAddr(vaddr).SetPageBaseAddr(paddr, attr);
  }
  void Print(void);
  void DebugPrintEntryForAddr(uint64_t vaddr);
  uint64_t v2p(uint64_t vaddr) {
    EntryType& e = GetEntryForAddr(vaddr);
    if (!e.IsPresent())
      return kAddrCannotTranslate;
    if (e.IsPage())
      return (e.GetPageBaseAddr()) | (vaddr & kOffsetMask);
    return e.v2pWithTable(vaddr);
  }
};

template <int TIndexShift, class TPageStrategy>
packed_struct PageTableEntryStruct {
  using Strategy = TPageStrategy;
  static constexpr int kIndexShift = TIndexShift;
  static constexpr uint64_t kOffsetMask = (1ULL << kIndexShift) - 1;
  uint64_t data;
  bool IsPresent() { return data & kPageAttrPresent; }
  template <class S = TPageStrategy>
  typename S::NextTableType* GetTableAddr() {
    if (IsPage())
      return nullptr;
    return reinterpret_cast<typename S::NextTableType*>(
        data & GetPhysAddrMask() & ~kPageAddrMask);
  }
  template <class S = TPageStrategy>
  void SetTableAddr(typename S::NextTableType * table, uint64_t attr) {
    const uint64_t addr_mask = (GetPhysAddrMask() & ~kPageAddrMask);
    assert((reinterpret_cast<uint64_t>(table) & ~addr_mask) == 0);
    data = reinterpret_cast<uint64_t>(table) | attr;
    SetAttrAsTable();
  }
  uint64_t GetPageBaseAddr(void) {
    const uint64_t addr_mask = (GetPhysAddrMask() & ~kOffsetMask);
    return data & addr_mask;
  }
  void SetPageBaseAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & kOffsetMask) == 0);
    const uint64_t addr_mask = (GetPhysAddrMask() & ~kOffsetMask);
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
    SetAttrAsPage();
  }
  void SetAttr(uint64_t attr) {
    data &= ~kPageAttrMask;
    data |= kPageAttrMask & attr;
  }
  template <class S = Strategy>
  auto v2pWithTable(uint64_t vaddr)
      ->IsValidType<typename S::NextTableType, uint64_t> {
    return v2p(GetTableAddr(), vaddr);
  }
  uint64_t v2pWithTable(...) { return kAddrCannotTranslate; }

  template <typename S = Strategy>
  bool IsPage(char(*)[CanHavePageF<S>()] = 0, char(*)[CanHaveTableF<S>()] = 0) {
    return data & (1 << 7);
  }
  template <class S = Strategy>
  bool IsPage(char(*)[!CanHavePageF<S>()] = 0,
              char(*)[CanHaveTableF<S>()] = 0) {
    return false;
  }
  template <class S = Strategy>
  bool IsPage(char(*)[CanHavePageF<S>()] = 0,
              char(*)[!CanHaveTableF<S>()] = 0) {
    return true;
  }
  template <typename S = Strategy>
  void SetAttrAsPage(char(*)[CanHavePageF<S>()] = 0,
                     char(*)[CanHaveTableF<S>()] = 0) {
    data |= (1ULL << 7);
  }
  template <typename S = Strategy>
  void SetAttrAsPage(char(*)[CanHavePageF<S>()] = 0,
                     char(*)[!CanHaveTableF<S>()] = 0) {}
  template <typename S = Strategy>
  void SetAttrAsTable(char(*)[CanHavePageF<S>()] = 0,
                      char(*)[CanHaveTableF<S>()] = 0) {
    data &= ~(1ULL << 7);
  }
  template <typename S = Strategy>
  void SetAttrAsTable(char(*)[!CanHavePageF<S>()] = 0,
                      char(*)[CanHaveTableF<S>()] = 0) {}
};

struct PTEStrategy {
  static constexpr int kBitsOfOffsetInPage = 12;
  static inline void SetIsPage(uint64_t&, bool is_page) {
    if (!is_page)
      Panic("Not reached");
  }
};
static_assert(CanHavePageF<PTEStrategy>());
static_assert(!CannotHavePageF<PTEStrategy>());
static_assert(!CanHaveTableF<PTEStrategy>());
static_assert(CannotHaveTableF<PTEStrategy>());

using IA_PTE = PageTableEntryStruct<12, PTEStrategy>;
using IA_PT = PageTableStruct<IA_PTE>;
static_assert(sizeof(IA_PT) == kPageSize);

struct PDEStrategy {
  using NextTableType = IA_PT;
  static constexpr int kBitsOfOffsetInPage = 21;
};
static_assert(CanHavePageF<PDEStrategy>());
static_assert(!CannotHavePageF<PDEStrategy>());
static_assert(CanHaveTableF<PDEStrategy>());
static_assert(!CannotHaveTableF<PDEStrategy>());

using IA_PDE = PageTableEntryStruct<21, PDEStrategy>;
using IA_PDT = PageTableStruct<IA_PDE>;
static_assert(sizeof(IA_PDT) == kPageSize);

struct PDPTEStrategy {
  using NextTableType = IA_PDT;
  static constexpr int kBitsOfOffsetInPage = 30;
};
using IA_PDPTE = PageTableEntryStruct<30, PDPTEStrategy>;
using IA_PDPT = PageTableStruct<IA_PDPTE>;
static_assert(sizeof(IA_PDPT) == kPageSize);

struct PML4EStrategy {
  using NextTableType = IA_PDPT;
};
using IA_PML4E = PageTableEntryStruct<39, PML4EStrategy>;
using IA_PML4 = PageTableStruct<IA_PML4E>;
static_assert(sizeof(IA_PML4) == kPageSize);

IA_PML4& CreatePageTable();
void InitPaging(void);
IA_PML4& GetKernelPML4(void);

void inline CreatePageMapping(PhysicalPageAllocator& allocator,
                              IA_PML4& pml4,
                              uint64_t vaddr,
                              uint64_t paddr,
                              uint64_t byte_size,
                              uint64_t attr) {
  assert((vaddr & kPageAddrMask) == 0);
  assert((paddr & kPageAddrMask) == 0);
  uint64_t num_of_4k_pages = ByteSizeToPageSize(byte_size);
  for (int pml4_idx = IA_PML4::addr2index(vaddr);
       pml4_idx < IA_PML4::kNumOfEntries; pml4_idx++) {
    auto& pml4e = pml4.GetEntryForAddr(vaddr);
    if (!pml4e.IsPresent()) {
      IA_PDPT* new_pdpt = allocator.AllocPages<IA_PDPT*>(1);
      new_pdpt->ClearMapping();
      pml4e.SetTableAddr(new_pdpt, attr);
    }
    pml4e.SetAttr(attr);
    auto* pdpt = pml4e.GetTableAddr();
    for (int pdpt_idx = IA_PDPT::addr2index(vaddr);
         num_of_4k_pages && pdpt_idx < IA_PDPT::kNumOfEntries; pdpt_idx++) {
      auto& pdpte = pdpt->GetEntryForAddr(vaddr);
      if (!pdpte.IsPresent()) {
        IA_PDT* new_pdt = allocator.AllocPages<IA_PDT*>(1);
        new_pdt->ClearMapping();
        pdpte.SetTableAddr(new_pdt, attr);
      }
      pdpte.SetAttr(attr);
      if (pdpte.IsPage())
        Panic("Page overwrapping at pdpte");
      auto* pdt = pdpte.GetTableAddr();
      for (int pdt_idx = IA_PDT::addr2index(vaddr);
           num_of_4k_pages && pdt_idx < IA_PDT::kNumOfEntries; pdt_idx++) {
        auto& pdte = pdt->GetEntryForAddr(vaddr);
        if (!pdte.IsPresent()) {
          if (num_of_4k_pages >= IA_PT::kNumOfEntries &&
              (vaddr & IA_PDE::kOffsetMask) == 0 &&
              (paddr & IA_PDE::kOffsetMask) == 0) {
            // 2MB mapping
            pdte.SetPageBaseAddr(paddr, attr);
            vaddr += (1 << 21);
            paddr += (1 << 21);
            num_of_4k_pages -= IA_PT::kNumOfEntries;
            continue;
          }
          IA_PT* new_pt = allocator.AllocPages<IA_PT*>(1);
          new_pt->ClearMapping();
          pdte.SetTableAddr(new_pt, attr);
        }
        pdte.SetAttr(attr);
        if (pdte.IsPage())
          Panic("Page overwrapping at pdte");
        auto* pt = pdte.GetTableAddr();
        for (int pt_idx = IA_PT::addr2index(vaddr);
             num_of_4k_pages && pt_idx < IA_PT::kNumOfEntries; pt_idx++) {
          pt->SetPageBaseForAddr(vaddr, paddr, attr);
          vaddr += (1 << 12);
          paddr += (1 << 12);
          num_of_4k_pages--;
        }
      }
    }
  }
}

template <typename TableType>
uint64_t v2p(TableType& table, uint64_t vaddr) {
  return v2p(&table, vaddr);
}
template <typename TableType>
uint64_t v2p(TableType* table, uint64_t vaddr) {
  return table->v2p(vaddr);
}
