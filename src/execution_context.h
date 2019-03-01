#pragma once

#include "cpu_context.h"

class ExecutionContext {
 public:
  enum class Status {
    kNotScheduled,
    kSleeping,
    kRunning,
    kKilled,
  };
  ExecutionContext(uint64_t id,
                   void (*rip)(),
                   uint16_t cs,
                   void* rsp,
                   uint16_t ss,
                   uint64_t cr3)
      : id_(id), status_(Status::kNotScheduled) {
    cpu_context_.int_info.rip = reinterpret_cast<uint64_t>(rip);
    cpu_context_.int_info.cs = cs;
    cpu_context_.int_info.rsp = reinterpret_cast<uint64_t>(rsp);
    cpu_context_.int_info.ss = ss;
    cpu_context_.int_info.eflags = 0x202;
    cpu_context_.cr3 = cr3;
  }
  uint64_t GetID() { return id_; };
  int GetSchedulerIndex() const { return scheduler_index_; };
  void SetSchedulerIndex(int scheduler_index) {
    scheduler_index_ = scheduler_index;
  };
  CPUContext* GetCPUContext() { return &cpu_context_; };
  Status GetStatus() const { return status_; };
  void SetStatus(Status status) { status_ = status; };

 private:
  uint64_t id_;
  int scheduler_index_;
  Status status_;
  CPUContext cpu_context_;
};
