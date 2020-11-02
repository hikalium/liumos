#pragma once

#include "generic.h"
#include "paging.h"
#include "phys_page_allocator.h"

class KernelVirtualHeapAllocator {
 public:
  KernelVirtualHeapAllocator(IA_PML4& pml4,
                             KernelPhysPageAllocator& dram_allocator)
      : next_base_(kKernelHeapBaseAddr),
        pml4_(pml4),
        dram_allocator_(dram_allocator){};
  template <typename T>
  T AllocPages(uint64_t num_of_pages) {
    // Returns a memory region writable && present (in the kernel straight
    // mapping). This function is safe to be called under a user mappings.
    return reinterpret_cast<T>(
        dram_allocator_.AllocPages<uint64_t>(num_of_pages) +
        GetKernelStraightMappingBase());
  }
  template <typename T>
  T MapPages(uint64_t paddr, uint64_t num_of_pages, uint64_t page_attr) {
    uint64_t byte_size = (num_of_pages << kPageSizeExponent);
    if (byte_size > kKernelHeapSize ||
        next_base_ + byte_size > kKernelHeapBaseAddr + kKernelHeapSize)
      Panic("Cannot allocate kernel virtual heap");
    uint64_t vaddr = next_base_;
    next_base_ += byte_size + (1 << kPageSizeExponent);
    CreatePageMapping(dram_allocator_, pml4_, vaddr, paddr, byte_size,
                      page_attr);
    return reinterpret_cast<T>(vaddr);
  }

  template <typename T>
  T* Alloc() {
    // Returns a memory region writable && present (in the kernel straight
    // mapping). This function is safe to be called under a user mappings.
    return AllocPages<T*>(ByteSizeToPageSize(sizeof(T)));
  }

 private:
  static constexpr uint64_t kKernelHeapBaseAddr = 0xFFFF'FFFF'9000'0000;
  static constexpr uint64_t kKernelHeapSize = 0x0000'0000'4000'0000;
  uint64_t next_base_;
  IA_PML4& pml4_;
  KernelPhysPageAllocator& dram_allocator_;
};
