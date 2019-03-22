#pragma once

#include "generic.h"

#include "execution_context.h"
#include "kernel_virtual_heap_allocator.h"

class Process {
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
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; }
  void WaitUntilExit();
  void InitAsEphemeralProcess(ExecutionContext& ctx) { ctx_ = &ctx; }
  ExecutionContext& GetExecutionContext() {
    assert(ctx_);
    return *ctx_;
  }
  friend class ProcessController;

 private:
  Process(uint64_t id) : id_(id){};
  uint64_t id_;
  volatile Status status_;
  int scheduler_index_;
  ExecutionContext* ctx_;
};

class ProcessController {
 public:
  ProcessController(KernelVirtualHeapAllocator& kernel_heap_allocator)
      : last_id_(0), kernel_heap_allocator_(kernel_heap_allocator){};
  Process& Create();

 private:
  uint64_t last_id_;
  KernelVirtualHeapAllocator& kernel_heap_allocator_;
};
