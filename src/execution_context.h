#pragma once

#include "asm.h"
#include "kernel_virtual_heap_allocator.h"
#include "paging.h"

struct SegmentMapping {
  uint64_t paddr;
  uint64_t vaddr;
  uint64_t size;
  void Print();
  void Clear() {
    paddr = 0;
    vaddr = 0;
    size = 0;
  }
};

struct ProcessMappingInfo {
  SegmentMapping code;
  SegmentMapping data;
  SegmentMapping stack;
  void Print();
  void Clear() {
    code.Clear();
    data.Clear();
    stack.Clear();
  }
};

class ExecutionContext {
 public:
  CPUContext* GetCPUContext() { return &cpu_context_; }
  uint64_t GetKernelRSP() { return kernel_rsp_; }
  friend class ExecutionContextController;

 private:
  ExecutionContext(void (*rip)(),
                   uint16_t cs,
                   void* rsp,
                   uint16_t ss,
                   uint64_t cr3,
                   uint64_t rflags,
                   uint64_t kernel_rsp) {
    cpu_context_.int_ctx.rip = reinterpret_cast<uint64_t>(rip);
    cpu_context_.int_ctx.cs = cs;
    cpu_context_.int_ctx.rsp = reinterpret_cast<uint64_t>(rsp);
    cpu_context_.int_ctx.ss = ss;
    cpu_context_.int_ctx.rflags = rflags | 2;
    cpu_context_.cr3 = cr3;
    kernel_rsp_ = kernel_rsp;
  }
  CPUContext cpu_context_;
  uint64_t kernel_rsp_;
};

class ExecutionContextController {
 public:
  ExecutionContextController(KernelVirtualHeapAllocator& kernel_heap_allocator)
      : kernel_heap_allocator_(kernel_heap_allocator){};
  ExecutionContext& Create(void (*rip)(),
                           uint16_t cs,
                           void* rsp,
                           uint16_t ss,
                           uint64_t cr3,
                           uint64_t rflags,
                           uint64_t kernel_rsp);

 private:
  KernelVirtualHeapAllocator& kernel_heap_allocator_;
};

struct PersistentProcessInfo {
  ExecutionContext ctx[3];
  int save_completed_ctx_idx;
};
