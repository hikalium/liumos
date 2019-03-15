#pragma once

#include "asm.h"
#include "kernel_virtual_heap_allocator.h"
#include "paging.h"

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
