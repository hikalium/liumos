#pragma once

#include "asm.h"
#include "kernel_virtual_heap_allocator.h"
#include "paging.h"

class PersistentMemoryManager;

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
  void AllocSegmentFromPersistentMemory(PersistentMemoryManager& pmem);
  void Print();
  void CopyDataFrom(SegmentMapping& from);
  void Map(IA_PML4& page_root, uint64_t attr);
  void Flush();

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
  void Flush() {
    code.Flush();
    data.Flush();
    stack.Flush();
  }
};

class ExecutionContext {
 public:
  CPUContext& GetCPUContext() { return cpu_context_; }
  ProcessMappingInfo& GetProcessMappingInfo() { return map_info_; };
  uint64_t GetKernelRSP() { return kernel_rsp_; }
  void SetKernelRSP(uint64_t kernel_rsp) { kernel_rsp_ = kernel_rsp; }
  void SetCR3(uint64_t cr3) { cpu_context_.cr3 = cr3; }
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
  void Flush();
  void CopyContextFrom(ExecutionContext& from) {
    uint64_t cr3 = cpu_context_.cr3;
    cpu_context_ = from.cpu_context_;
    cpu_context_.cr3 = cr3;

    map_info_.data.CopyDataFrom(from.map_info_.data);
    map_info_.stack.CopyDataFrom(from.map_info_.stack);
  }

 private:
  CPUContext cpu_context_;
  ProcessMappingInfo map_info_;
  uint64_t kernel_rsp_;
};

class PersistentProcessInfo {
 public:
  bool IsValid() { return signature_ == kSignature; }
  void Print();
  void Init() {
    valid_ctx_idx_ = kNumOfExecutionContext;
    CLFlush(&valid_ctx_idx_);
    signature_ = kSignature;
    CLFlush(&signature_);
  }
  ExecutionContext& GetContext(int idx) {
    assert(0 <= idx && idx < kNumOfExecutionContext);
    return ctx_[idx];
  }
  ExecutionContext& GetValidContext() {
    assert(0 <= valid_ctx_idx_ && valid_ctx_idx_ < kNumOfExecutionContext);
    return ctx_[valid_ctx_idx_];
  }
  ExecutionContext& GetWorkingContext() {
    assert(0 <= valid_ctx_idx_ && valid_ctx_idx_ < kNumOfExecutionContext);
    return ctx_[1 - valid_ctx_idx_];
  }
  void SetValidContextIndex(int idx) {
    valid_ctx_idx_ = idx;
    CLFlush(&valid_ctx_idx_);
  }
  static constexpr uint64_t kSignature = 0x4F50534F6D75696CULL;
  static constexpr int kNumOfExecutionContext = 2;
  void SwitchContext();

 private:
  ExecutionContext ctx_[kNumOfExecutionContext];
  int valid_ctx_idx_;
  uint64_t signature_;
};
