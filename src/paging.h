#pragma once

#include "generic.h"
#include "phys_page_allocator.h"

extern int kMaxPhyAddr;

constexpr uint64_t kAddrCannotTranslate =
    0x8000'0000'0000'0000;  // non-canonical address

constexpr uint64_t kPageAttrMask = 0b11111;
constexpr uint64_t kPageAttrPresent = 0b00001;
constexpr uint64_t kPageAttrWritable = 0b00010;
constexpr uint64_t kPageAttrUser = 0b00100;
constexpr uint64_t kPageAttrWriteThrough = 0b01000;
constexpr uint64_t kPageAttrCacheDisable = 0b10000;

struct IA_PML4E;

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

template <int index_shift, typename ET>
struct PageTableStruct {
  using EntryType = ET;
  static constexpr int kNumOfEntries = (1 << 9);
  static constexpr int kIndexMask = kNumOfEntries - 1;
  static constexpr int kIndexShift = index_shift;
  static constexpr uint64_t kOffsetMask = (1ULL << kIndexShift) - 1;
  static inline int addr2index(uint64_t addr) {
    return (addr >> kIndexShift) & kIndexMask;
  }
  EntryType entries[kNumOfEntries];
  EntryType& GetEntryForAddr(uint64_t vaddr) {
    return entries[addr2index(vaddr)];
  }
  void ClearMapping() {
    for (int i = 0; i < kNumOfEntries; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  typename EntryType::TableType* GetTableBaseForAddr(uint64_t vaddr) {
    return GetEntryForAddr(vaddr).GetTableAddr();
  }
  typename EntryType::TableType* SetTableBaseForAddr(
      uint64_t addr,
      typename EntryType::TableType* new_ent,
      uint64_t attr) {
    EntryType& e = GetEntryForAddr(addr);
    typename EntryType::TableType* old_e = e.GetTableAddr();
    e.SetTableAddr(new_ent, attr);
    return old_e;
  }
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    GetEntryForAddr(vaddr).SetPageBaseAddr(paddr, attr);
  }
  void Print(void);
};

packed_struct IA_PT_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t ignored0 : 1;
  uint64_t PAT : 1;
  uint64_t ignored1 : 4;
  uint64_t page_table_addr_and_reserved : 40;
  uint64_t ignored2 : 11;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PT_BITS) == 8);

packed_struct IA_PTE {
  using TableType = void;
  union {
    uint64_t data;
    IA_PT_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
  bool IsPage() { return true; }
  uint64_t GetPageBaseAddr(void) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    return data & addr_mask;
  }
  void SetPageBaseAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & ((1ULL << 12) - 1)) == 0);
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
  }
};

using IA_PT = PageTableStruct<12, IA_PTE>;
template <>
inline uint64_t v2p<IA_PT>(IA_PT* table, uint64_t vaddr) {
  IA_PTE& e = table->entries[IA_PT::addr2index(vaddr)];
  if (!e.IsPresent())
    return kAddrCannotTranslate;
  return (e.GetPageBaseAddr()) | (vaddr & IA_PT::kOffsetMask);
}

packed_struct IA_PDE_2MB_PAGE_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t is_dirty : 1;
  uint64_t is_2mb_page : 1;
  uint64_t is_global : 1;
  uint64_t ignored0 : 3;
  uint64_t PAT : 1;
  uint64_t page_frame_addr_and_reserved : 37;
  uint64_t ignored1 : 7;
  uint64_t protection_key : 4;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PDE_2MB_PAGE_BITS) == 8);

packed_struct IA_PDE_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t ignored0 : 1;
  uint64_t is_2mb_page : 1;
  uint64_t ignored1 : 4;
  uint64_t page_table_addr_and_reserved : 40;
  uint64_t ignored2 : 11;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PDE_BITS) == 8);

packed_struct IA_PDE {
  using TableType = IA_PT;
  union {
    uint64_t data;
    IA_PDE_2MB_PAGE_BITS page_bits;
    IA_PDE_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
  bool IsPage() { return bits.is_2mb_page; }
  IA_PT* GetTableAddr() {
    if (IsPage())
      return nullptr;
    return reinterpret_cast<IA_PT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                    ~((1ULL << 12) - 1));
  }
  void SetTableAddr(IA_PT * table, uint64_t attr) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    assert((reinterpret_cast<uint64_t>(table) & ~addr_mask) == 0);
    data = reinterpret_cast<uint64_t>(table) | attr;
  }
  uint64_t GetPageBaseAddr(void) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 21) - 1));
    return data & addr_mask;
  }
  void SetPageBaseAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & ((1ULL << 21) - 1)) == 0);
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 21) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
    bits.is_2mb_page = 1;
  }
};

using IA_PDT = PageTableStruct<21, IA_PDE>;

packed_struct IA_PDPTE_1GB_PAGE_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t is_dirty : 1;
  uint64_t is_1gb_page : 1;
  uint64_t is_global : 1;
  uint64_t ignored0 : 3;
  uint64_t PAT : 1;
  uint64_t page_frame_addr_and_reserved : 37;
  uint64_t ignored1 : 7;
  uint64_t protection_key : 4;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PDPTE_1GB_PAGE_BITS) == 8);

packed_struct IA_PDPTE_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t ignored0 : 1;
  uint64_t is_1gb_page : 1;
  uint64_t ignored1 : 4;
  uint64_t page_table_addr_and_reserved : 40;
  uint64_t ignored2 : 11;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PDPTE_BITS) == 8);

packed_struct IA_PDPTE {
  using TableType = IA_PDT;
  union {
    uint64_t data;
    IA_PDPTE_1GB_PAGE_BITS page_bits;
    IA_PDPTE_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
  bool IsPage() { return bits.is_1gb_page; }
  IA_PDT* GetTableAddr() {
    if (IsPage())
      return nullptr;
    return reinterpret_cast<IA_PDT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                     ~((1ULL << 12) - 1));
  }
  void SetTableAddr(IA_PDT * pdt, uint64_t attr) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    assert((reinterpret_cast<uint64_t>(pdt) & ~addr_mask) == 0);
    data = reinterpret_cast<uint64_t>(pdt) | attr;
  }
  void SetPageBaseAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & ((1ULL << 30) - 1)) == 0);
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
    bits.is_1gb_page = 1;
  }
  uint64_t GetPageBaseAddr(void) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    return data & addr_mask;
  }
};

using IA_PDPT = PageTableStruct<30, IA_PDPTE>;

packed_struct IA_PML4E_BITS {
  uint64_t is_present : 1;
  uint64_t is_writable : 1;
  uint64_t is_accessible_from_user_mode : 1;
  uint64_t page_level_write_through : 1;
  uint64_t page_level_cache_disable : 1;
  uint64_t is_accessed : 1;
  uint64_t ignored0 : 1;
  uint64_t reserved0 : 1;
  uint64_t ignored1 : 4;
  uint64_t pdpt_addr_with_reserved_bits : 51;
  uint64_t is_execute_disabled : 1;
};
static_assert(sizeof(IA_PML4E_BITS) == 8);

packed_struct IA_PML4E {
  using TableType = IA_PDPT;
  union {
    uint64_t data;
    IA_PML4E_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
  bool IsPage() { return false; }
  IA_PDPT* GetTableAddr() {
    return reinterpret_cast<IA_PDPT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                      ~((1ULL << 12) - 1));
  }
  void SetTableAddr(IA_PDPT * pdpt, uint64_t attr) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (reinterpret_cast<uint64_t>(pdpt) & addr_mask) | (attr);
  }
  uint64_t GetPageBaseAddr(void) { return 0; };
};

using IA_PML4 = PageTableStruct<39, IA_PML4E>;

IA_PML4* CreatePageTable();
void InitPaging(void);

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
    auto* pdpt = pml4e.GetTableAddr();
    for (int pdpt_idx = IA_PDPT::addr2index(vaddr);
         num_of_4k_pages && pdpt_idx < IA_PDPT::kNumOfEntries; pdpt_idx++) {
      auto& pdpte = pdpt->GetEntryForAddr(vaddr);
      if (!pdpte.IsPresent()) {
        pdpte.SetTableAddr(allocator.AllocPages<IA_PDT*>(1), attr);
      }
      if (pdpte.IsPage())
        Panic("Page overwrapping");
      auto* pdt = pdpte.GetTableAddr();
      for (int pdt_idx = IA_PDT::addr2index(vaddr);
           num_of_4k_pages && pdt_idx < IA_PDT::kNumOfEntries; pdt_idx++) {
        auto& pdte = pdt->GetEntryForAddr(vaddr);
        if (!pdte.IsPresent()) {
          pdte.SetTableAddr(allocator.AllocPages<IA_PT*>(1), attr);
        }
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
