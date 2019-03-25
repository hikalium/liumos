#pragma once

#include "generic.h"

#include "execution_context.h"
#include "kernel_virtual_heap_allocator.h"

class Process {
 public:
  enum class Status {
    kNotInitialized,
    kNotScheduled,
    kSleeping,
    kRunning,
    kKilled,
  };
  bool IsPersistent() {
    if (ctx_) {
      assert(!pp_info_);
      return false;
    }
    assert(pp_info_);
    return true;
  }
  uint64_t GetID() { return id_; }
  int GetSchedulerIndex() const { return scheduler_index_; }
  void SetSchedulerIndex(int scheduler_index) {
    scheduler_index_ = scheduler_index;
  };
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; }
  void WaitUntilExit();
  void InitAsEphemeralProcess(ExecutionContext& ctx) {
    assert(status_ == Status::kNotInitialized);
    assert(!ctx_);
    assert(!pp_info_);
    ctx_ = &ctx;
    status_ = Status::kNotScheduled;
  }
  void InitAsPersistentProcess(PersistentProcessInfo& pp_info) {
    assert(status_ == Status::kNotInitialized);
    assert(!ctx_);
    assert(!pp_info_);
    pp_info_ = &pp_info;
    status_ = Status::kNotScheduled;
  }
  ExecutionContext& GetExecutionContext() {
    return IsPersistent() ? pp_info_->GetWorkingContext() : *ctx_;
  }
  void NotifyContextSaving();
  friend class ProcessController;

 private:
  Process(uint64_t id)
      : id_(id),
        status_(Status::kNotInitialized),
        ctx_(nullptr),
        pp_info_(nullptr){};
  uint64_t id_;
  volatile Status status_;
  int scheduler_index_;
  ExecutionContext* ctx_;
  PersistentProcessInfo* pp_info_;
};

class ProcessController {
 public:
  ProcessController(KernelVirtualHeapAllocator& kernel_heap_allocator)
      : last_id_(0), kernel_heap_allocator_(kernel_heap_allocator){};
  Process& Create();
  Process& RestoreFromPersistentProcessInfo(PersistentProcessInfo& pp_info);

 private:
  uint64_t last_id_;
  KernelVirtualHeapAllocator& kernel_heap_allocator_;
};
