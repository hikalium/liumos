#pragma once

#include "execution_context.h"
#include "generic.h"
#include "kernel_virtual_heap_allocator.h"
#include "ring_buffer.h"

class Process {
 public:
  using PID = uint64_t;
  enum class Status {
    kNotInitialized,
    kNotScheduled,
    kSleeping,
    kRunning,
    kStopping,
    kStopped,
  };
  bool IsPersistent() {
    if (ctx_) {
      assert(!pp_info_);
      return false;
    }
    assert(pp_info_);
    return true;
  }
  PID GetID() { return id_; }
  const char* GetName() { return name_; }
  int GetSchedulerIndex() const { return scheduler_index_; }
  void SetSchedulerIndex(int scheduler_index) {
    scheduler_index_ = scheduler_index;
  };
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; }
  void Kill();
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
  uint64_t GetNumberOfContextSwitch() { return number_of_ctx_switch_; }
  uint64_t GetProcTimeFemtoSec() { return proc_time_femto_sec_; }
  void ResetProcTimeFemtoSec() { proc_time_femto_sec_ = 0; }
  void AddProcTimeFemtoSec(uint64_t fs) { proc_time_femto_sec_ += fs; }
  uint64_t GetSysTimeFemtoSec() { return sys_time_femto_sec_; }
  void ResetSysTime() { sys_time_femto_sec_ = 0; }
  void AddSysTimeFemtoSec(uint64_t fs) { sys_time_femto_sec_ += fs; }
  void AddTimeConsumedInContextSavingFemtoSec(uint64_t fs) {
    time_consumed_in_ctx_save_femto_sec_ += fs;
  }
  void PrintStatistics();
  RingBuffer<uint8_t, 16>& GetStdIn() { return stdin_buffer_; }

  friend class ProcessController;

 private:
  Process(uint64_t id, const char* name)
      : id_(id),
        name_(name),
        status_(Status::kNotInitialized),
        ctx_(nullptr),
        pp_info_(nullptr),
        number_of_ctx_switch_(0),
        proc_time_femto_sec_(0),
        sys_time_femto_sec_(0),
        copied_bytes_in_ctx_sw_(0),
        num_of_clflush_issued_in_ctx_sw_(0),
        time_consumed_in_ctx_save_femto_sec_(0){};
  uint64_t id_;
  const char* name_;
  volatile Status status_;
  int scheduler_index_;
  ExecutionContext* ctx_;
  PersistentProcessInfo* pp_info_;
  uint64_t number_of_ctx_switch_;
  uint64_t proc_time_femto_sec_;
  uint64_t sys_time_femto_sec_;
  uint64_t copied_bytes_in_ctx_sw_;
  uint64_t num_of_clflush_issued_in_ctx_sw_;
  uint64_t time_consumed_in_ctx_save_femto_sec_;
  RingBuffer<uint8_t, 16> stdin_buffer_;
};

class ProcessController {
 public:
  ProcessController(KernelVirtualHeapAllocator& kernel_heap_allocator)
      : last_id_(0), kernel_heap_allocator_(kernel_heap_allocator){};
  Process& Create(const char* const name);
  Process& RestoreFromPersistentProcessInfo(PersistentProcessInfo& pp_info);

 private:
  uint64_t last_id_;
  KernelVirtualHeapAllocator& kernel_heap_allocator_;
};
