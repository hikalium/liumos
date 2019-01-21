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

packed_struct IA_PT;

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
  bool Is2MBPage() { return bits.is_2mb_page; }
  IA_PT* GetPTAddr() {
    return reinterpret_cast<IA_PT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                    ~((1ULL << 12) - 1));
  }
};

packed_struct IA_PDT {
  static constexpr int kNumOfPDE = (1 << 9);
  IA_PDE entries[kNumOfPDE];
  void Print();
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
  bool Is1GBPage() { return bits.is_1gb_page; }
  IA_PDT* GetPDTAddr() {
    return reinterpret_cast<IA_PDT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                     ~((1ULL << 12) - 1));
  }
  void Set1GBPageAddr(uint64_t paddr, uint64_t attr) {
    assert((paddr & ((1ULL << 30) - 1)) == 0);
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    data &= ~(addr_mask | kPageAttrMask);
    data |= (paddr & addr_mask) | attr;
    bits.is_1gb_page = 1;
  }
  uint64_t Get1GBPageAddr(void) {
    const uint64_t addr_mask =
        (((1ULL << kMaxPhyAddr) - 1) & ~((1ULL << 30) - 1));
    return data & addr_mask;
  }
};

packed_struct IA_PDPT {
  static constexpr int kNumOfPDPTE = (1 << 9);
  IA_PDPTE entries[kNumOfPDPTE];
  void Print();
  uint64_t v2p(uint64_t vaddr) {
    IA_PDPTE& pdpte = entries[(vaddr >> 30) & ((1 << 9) - 1)];
    if (!pdpte.IsPresent())
      return kAddrCannotTranslate;
    if (pdpte.Is1GBPage())
      return (pdpte.Get1GBPageAddr()) | (vaddr & ((1ULL << 30) - 1));
    return 0;
  };
  void Set1GBPageForAddr(uint64_t vaddr, uint64_t paddr, uint64_t attr) {
    IA_PDPTE& pdpte = entries[(vaddr >> 30) & ((1 << 9) - 1)];
    pdpte.Set1GBPageAddr(paddr, attr);
  }
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
  IA_PDPT* GetPDPTAddr() {
    return reinterpret_cast<IA_PDPT*>(data & ((1ULL << kMaxPhyAddr) - 1) &
                                      ~((1ULL << 12) - 1));
  }
  void SetPDPTAddr(IA_PDPT * pdpt, uint64_t attr) {
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
  uint64_t v2p(uint64_t addr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    if (!pml4e.IsPresent())
      return kAddrCannotTranslate;
    return pml4e.GetPDPTAddr()->v2p(addr);
  };
  IA_PDPT* GetPDPTForAddr(uint64_t addr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    return pml4e.GetPDPTAddr();
  }
  IA_PDPT* SetPDPTForAddr(uint64_t addr, IA_PDPT * new_ent, uint64_t attr) {
    IA_PML4E& pml4e = entries[(addr >> 39) & ((1 << 9) - 1)];
    IA_PDPT* old_ent = pml4e.GetPDPTAddr();
    pml4e.SetPDPTAddr(new_ent, attr);
    return old_ent;
  }
};
