#pragma once

#include "asm.h"
#include "kernel_virtual_heap_allocator.h"
#include "paging.h"

class SegmentMapping {
 public:
  void Set(uint64_t vaddr, uint64_t paddr, uint64_t map_size) {
    vaddr_ = vaddr;
    paddr_ = paddr;
    map_size_ = map_size;
    CLFlush(this);
  }
  uint64_t GetPhysAddr() { return paddr_; }
  void SetPhysAddr(uint64_t paddr) {
    paddr_ = paddr;
    CLFlush(&paddr);
  }
  uint64_t GetVirtAddr() { return vaddr_; }
  uint64_t GetMapSize() { return map_size_; }
  uint64_t GetVirtEndAddr() { return vaddr_ + map_size_; }
  void Clear() {
    paddr_ = 0;
    vaddr_ = 0;
    map_size_ = 0;
    CLFlush(this);
  }
  void Print();

 private:
  uint64_t vaddr_;
  uint64_t paddr_;
  uint64_t map_size_;
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
  ProcessMappingInfo& GetProcessMappingInfo() { return map_info_; };
  uint64_t GetKernelRSP() { return kernel_rsp_; }
  void SetRegisters(void (*rip)(),
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

 private:
  CPUContext cpu_context_;
  ProcessMappingInfo map_info_;
  uint64_t kernel_rsp_;
};

struct PersistentProcessInfo {
  bool IsValid() { return signature_ == kSignature; }
  void Print();
  void Init() {
    signature_ = kSignature;
    CLFlush(&signature_);
  }
  static constexpr uint64_t kSignature = 0x4F50534F6D75696CULL;
  ExecutionContext ctx[2];
  uint64_t signature_;
};
