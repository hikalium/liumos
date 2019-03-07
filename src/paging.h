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

template <typename TableType>
uint64_t v2p(TableType& table, uint64_t vaddr) {
  return v2p(&table, vaddr);
}
template <typename TableType>
uint64_t v2p(TableType* table, uint64_t vaddr) {
  typename TableType::EntryType& e =
      table->entries[TableType::addr2index(vaddr)];
  if (!e.IsPresent())
    return kAddrCannotTranslate;
  if (e.IsPage())
    return (e.GetPageBaseAddr()) | (vaddr & TableType::kOffsetMask);
  return v2p(e.GetTableAddr(), vaddr);
}
template <>
inline uint64_t v2p(void*, uint64_t) {
  return kAddrCannotTranslate;
}

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
  typename TEntryType::NextTableType* GetTableBaseForAddr(uint64_t vaddr) {
    return GetEntryForAddr(vaddr).GetTableAddr();
  }
  typename TEntryType::NextTableType* SetTableBaseForAddr(
      uint64_t addr,
      typename TEntryType::NextTableType* new_ent,
      uint64_t attr) {
    TEntryType& e = GetEntryForAddr(addr);
    typename TEntryType::NextTableType* old_e = e.GetTableAddr();
    e.SetTableAddr(new_ent, attr);
    return old_e;
  }
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    GetEntryForAddr(vaddr).SetPageBaseAddr(paddr, attr);
  }
  void Print(void);
};

template <int TIndexShift, class TPageStrategy>
packed_struct PageTableEntryStruct {
  using NextTableType = typename TPageStrategy::NextTableType;
  static constexpr int kIndexShift = TIndexShift;
  static constexpr uint64_t kOffsetMask = (1ULL << kIndexShift) - 1;
  uint64_t data;
  bool IsPresent() { return data & kPageAttrPresent; }
  bool IsPage() { return TPageStrategy::IsPage(data); }
  typename TPageStrategy::NextTableType* GetTableAddr() {
    if (IsPage())
      return nullptr;
    return reinterpret_cast<typename TPageStrategy::NextTableType*>(
        data & GetPhysAddrMask() & ~kPageAddrMask);
  }
  void SetTableAddr(typename TPageStrategy::NextTableType * table,
                    uint64_t attr) {
    const uint64_t addr_mask = (GetPhysAddrMask() & ~kPageAddrMask);
    assert((reinterpret_cast<uint64_t>(table) & ~addr_mask) == 0);
    data = reinterpret_cast<uint64_t>(table) | attr;
    TPageStrategy::SetIsPage(data, false);
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
    TPageStrategy::SetIsPage(data, true);
  }
  void SetAttr(uint64_t attr) {
    data &= ~kPageAttrMask;
    data |= kPageAttrMask & attr;
  }
};

struct PTEStrategy {
  using NextTableType = void;
  static inline bool IsPage(uint64_t) { return true; }
  static inline void SetIsPage(uint64_t&, bool is_page) {
    if (!is_page)
      Panic("Not reached");
  }
};
using IA_PTE = PageTableEntryStruct<12, PTEStrategy>;
using IA_PT = PageTableStruct<IA_PTE>;
static_assert(sizeof(IA_PT) == kPageSize);

struct PDEStrategy {
  using NextTableType = IA_PT;
  static inline bool IsPage(uint64_t data) { return data & (1 << 7); }
  static inline void SetIsPage(uint64_t& data, bool is_page) {
    data |= is_page ? 1 << 7 : 0;
  }
};
using IA_PDE = PageTableEntryStruct<21, PDEStrategy>;
using IA_PDT = PageTableStruct<IA_PDE>;
static_assert(sizeof(IA_PDT) == kPageSize);

struct PDPTEStrategy {
  using NextTableType = IA_PDT;
  static inline bool IsPage(uint64_t data) { return data & (1 << 7); }
  static inline void SetIsPage(uint64_t& data, bool is_page) {
    data |= is_page ? 1 << 7 : 0;
  }
};
using IA_PDPTE = PageTableEntryStruct<30, PDPTEStrategy>;
using IA_PDPT = PageTableStruct<IA_PDPTE>;
static_assert(sizeof(IA_PDPT) == kPageSize);

struct PML4EStrategy {
  using NextTableType = IA_PDPT;
  static inline bool IsPage(uint64_t) { return false; }
  static inline void SetIsPage(uint64_t&, bool is_page) {
    if (is_page)
      Panic("Not reached");
  }
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
      pml4e.SetTableAddr(allocator.AllocPages<IA_PDPT*>(1), attr);
    }
    pml4e.SetAttr(attr);
    auto* pdpt = pml4e.GetTableAddr();
    for (int pdpt_idx = IA_PDPT::addr2index(vaddr);
         num_of_4k_pages && pdpt_idx < IA_PDPT::kNumOfEntries; pdpt_idx++) {
      auto& pdpte = pdpt->GetEntryForAddr(vaddr);
      if (!pdpte.IsPresent()) {
        pdpte.SetTableAddr(allocator.AllocPages<IA_PDT*>(1), attr);
      }
      pdpte.SetAttr(attr);
      if (pdpte.IsPage())
        Panic("Page overwrapping");
      auto* pdt = pdpte.GetTableAddr();
      for (int pdt_idx = IA_PDT::addr2index(vaddr);
           num_of_4k_pages && pdt_idx < IA_PDT::kNumOfEntries; pdt_idx++) {
        auto& pdte = pdt->GetEntryForAddr(vaddr);
        if (!pdte.IsPresent()) {
          pdte.SetTableAddr(allocator.AllocPages<IA_PT*>(1), attr);
        }
        pdte.SetAttr(attr);
        if (pdte.IsPage())
          Panic("Page overwrapping");
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
