#pragma once

#include "asm.h"
#include "paging.h"

class KernelVirtualHeapAllocator {
 public:
  KernelVirtualHeapAllocator(IA_PML4& pml4,
                             PhysicalPageAllocator& dram_allocator)
      : next_base_(kKernelHeapBaseAddr),
        pml4_(pml4),
        dram_allocator_(dram_allocator){};
  template <typename T>
  T AllocPages(uint64_t num_of_pages, uint64_t page_attr) {
    uint64_t byte_size = (num_of_pages << kPageSizeExponent);
    if (byte_size > kKernelHeapSize ||
        next_base_ + byte_size > kKernelHeapBaseAddr + kKernelHeapSize)
      Panic("Cannot allocate kernel virtual heap");
    uint64_t vaddr = next_base_;
    next_base_ += byte_size + (1 << kPageSizeExponent);
    CreatePageMapping(dram_allocator_, pml4_, vaddr,
                      dram_allocator_.AllocPages<uint64_t>(num_of_pages),
                      byte_size, page_attr);
    return reinterpret_cast<T>(vaddr);
  }

 private:
  static constexpr uint64_t kKernelHeapBaseAddr = 0xFFFF'FFFF'9000'0000;
  static constexpr uint64_t kKernelHeapSize = 0x0000'0000'4000'0000;
  uint64_t next_base_;
  IA_PML4& pml4_;
  PhysicalPageAllocator& dram_allocator_;
};

class ExecutionContext {
 public:
  enum class Status {
    kNotScheduled,
    kSleeping,
    kRunning,
    kKilled,
  };
  uint64_t GetID() { return id_; }
  int GetSchedulerIndex() const { return scheduler_index_; }
  void SetSchedulerIndex(int scheduler_index) {
    scheduler_index_ = scheduler_index;
  };
  CPUContext* GetCPUContext() { return &cpu_context_; }
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; }
  uint64_t GetKernelRSP() { return kernel_rsp_; }

  friend class ExecutionContextController;
  void WaitUntilExit();

 private:
  ExecutionContext(uint64_t id,
                   void (*rip)(),
                   uint16_t cs,
                   void* rsp,
                   uint16_t ss,
                   uint64_t cr3,
                   uint64_t rflags,
                   uint64_t kernel_rsp)
      : id_(id), status_(Status::kNotScheduled), kernel_rsp_(kernel_rsp) {
    cpu_context_.int_ctx.rip = reinterpret_cast<uint64_t>(rip);
    cpu_context_.int_ctx.cs = cs;
    cpu_context_.int_ctx.rsp = reinterpret_cast<uint64_t>(rsp);
    cpu_context_.int_ctx.ss = ss;
    cpu_context_.int_ctx.rflags = rflags | 2;
    cpu_context_.cr3 = cr3;
  }
  uint64_t id_;
  int scheduler_index_;
  volatile Status status_;
  CPUContext cpu_context_;
  uint64_t kernel_rsp_;
};

class ExecutionContextController {
 public:
  ExecutionContextController(KernelVirtualHeapAllocator& kernel_heap_allocator)
      : last_context_id_(0), kernel_heap_allocator_(kernel_heap_allocator){};
  ExecutionContext* Create(void (*rip)(),
                           uint16_t cs,
                           void* rsp,
                           uint16_t ss,
                           uint64_t cr3,
                           uint64_t rflags,
                           uint64_t kernel_rsp);

 private:
  uint64_t last_context_id_;
  KernelVirtualHeapAllocator& kernel_heap_allocator_;
};
