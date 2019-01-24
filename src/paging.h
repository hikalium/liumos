#include "generic.h"

extern int kMaxPhyAddr;

constexpr uint64_t kAddrCannotTranslate =
    0x8000'0000'0000'0000;  // non-canonical address

constexpr uint64_t kPageAttrMask = 0b11111;
constexpr uint64_t kPageAttrPresent = 0b00001;
constexpr uint64_t kPageAttrWritable = 0b00010;
constexpr uint64_t kPageAttrUser = 0b00100;
constexpr uint64_t kPageAttrWriteThrough = 0b01000;
constexpr uint64_t kPageAttrCacheDisable = 0b10000;

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
  union {
    uint64_t data;
    IA_PT_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
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

packed_struct IA_PT {
  static constexpr int kNumOfPTE = (1 << 9);
  static constexpr int kIndexShift = 21;
  static constexpr int kIndexMask = kNumOfPTE - 1;
  static constexpr int kOffsetMask = (1ULL << kIndexShift) - 1;
  IA_PTE entries[kNumOfPTE];
  inline int addr2index(uint64_t addr) {
    return (addr >> kIndexShift) & kIndexMask;
  }
  void Print();
  void ClearMapping() {
    for (int i = 0; i < kNumOfPTE; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  uint64_t v2p(uint64_t vaddr) {
    IA_PTE& pte = entries[addr2index(vaddr)];
    if (!pte.IsPresent())
      return kAddrCannotTranslate;
    return (pte.GetPageBaseAddr()) | (vaddr & kOffsetMask);
  };
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    entries[addr2index(vaddr)].SetPageBaseAddr(paddr, attr);
  }
};

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

packed_struct IA_PDT {
  static constexpr int kNumOfPDE = (1 << 9);
  static constexpr int kIndexShift = 21;
  static constexpr int kIndexMask = kNumOfPDE - 1;
  static constexpr int kOffsetMask = (1ULL << kIndexShift) - 1;
  IA_PDE entries[kNumOfPDE];
  inline int addr2index(uint64_t addr) {
    return (addr >> kIndexShift) & kIndexMask;
  }
  void Print();
  void ClearMapping() {
    for (int i = 0; i < kNumOfPDE; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  uint64_t v2p(uint64_t vaddr) {
    IA_PDE& pde = entries[addr2index(vaddr)];
    if (!pde.IsPresent())
      return kAddrCannotTranslate;
    if (pde.IsPage())
      return (pde.GetPageBaseAddr()) | (vaddr & kOffsetMask);
    return pde.GetTableAddr()->v2p(vaddr);
  };
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    IA_PDE& pde = entries[addr2index(vaddr)];
    pde.SetPageBaseAddr(paddr, attr);
  }
  IA_PT* SetTableBaseForAddr(uint64_t addr, IA_PT * new_ent, uint64_t attr) {
    IA_PDE& pde = entries[addr2index(addr)];
    IA_PT* old_ent = pde.GetTableAddr();
    pde.SetTableAddr(new_ent, attr);
    return old_ent;
  }
};

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
  void SetTableAddr(IA_PDT * pdpt, uint64_t attr) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 12) - 1));
    assert((reinterpret_cast<uint64_t>(pdpt) & ~addr_mask) == 0);
    data = reinterpret_cast<uint64_t>(pdpt) | attr;
  }
  void SetPageAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & ((1ULL << 30) - 1)) == 0);
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
    bits.is_1gb_page = 1;
  }
  uint64_t GetPageAddr(void) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    return data & addr_mask;
  }
};

packed_struct IA_PDPT {
  static constexpr int kNumOfPDPTE = (1 << 9);
  static constexpr int kIndexMask = kNumOfPDPTE - 1;
  static constexpr int kIndexShift = 30;
  IA_PDPTE entries[kNumOfPDPTE];
  inline int addr2index(uint64_t addr) {
    return (addr >> kIndexShift) & kIndexMask;
  }
  void Print();
  void ClearMapping() {
    for (int i = 0; i < kNumOfPDPTE; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  uint64_t v2p(uint64_t vaddr) {
    IA_PDPTE& pdpte = entries[addr2index(vaddr)];
    if (!pdpte.IsPresent())
      return kAddrCannotTranslate;
    if (pdpte.IsPage())
      return (pdpte.GetPageAddr()) | (vaddr & ((1ULL << 30) - 1));
    return pdpte.GetTableAddr()->v2p(vaddr);
  };
  void SetPageBaseForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    IA_PDPTE& pdpte = entries[addr2index(vaddr)];
    pdpte.SetPageAddr(paddr, attr);
  }
  IA_PDT* SetTableBaseForAddr(uint64_t addr, IA_PDT * new_ent, uint64_t attr) {
    IA_PDPTE& pdpte = entries[addr2index(addr)];
    IA_PDT* old_ent = pdpte.GetTableAddr();
    pdpte.SetTableAddr(new_ent, attr);
    return old_ent;
  }

 private:
};

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
  union {
    uint64_t data;
    IA_PML4E_BITS bits;
  };
  bool IsPresent() { return bits.is_present; }
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
};

packed_struct IA_PML4 {
  static constexpr int kNumOfPML4E = (1 << 9);
  IA_PML4E entries[kNumOfPML4E];
  void Print();
  void ClearMapping() {
    for (int i = 0; i < kNumOfPML4E; i++) {
      reinterpret_cast<uint64_t*>(entries)[i] = 0;
    }
  }
  uint64_t v2p(uint64_t addr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    if (!pml4e.IsPresent())
      return kAddrCannotTranslate;
    return pml4e.GetTableAddr()->v2p(addr);
  };
  IA_PDPT* GetTableBaseForAddr(uint64_t addr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    return pml4e.GetTableAddr();
  }
  IA_PDPT* SetTableBaseForAddr(uint64_t addr, IA_PDPT * new_ent,
                               uint64_t attr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    IA_PDPT* old_ent = pml4e.GetTableAddr();
    pml4e.SetTableAddr(new_ent, attr);
    return old_ent;
  }
};

IA_PML4* CreatePageTable();
void InitPaging(void);
