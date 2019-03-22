#include "scheduler.h"
#include "liumos.h"

void Scheduler::RegisterProcess(Process& proc) {
  using Status = Process::Status;
  assert(number_of_process_ < kNumberOfProcess);
  assert(proc.GetStatus() == Process::Status::kNotScheduled);
  process_[number_of_process_] = &proc;
  proc.SetSchedulerIndex(number_of_process_);
  number_of_process_++;

  proc.SetStatus(Status::kSleeping);
}

Process* Scheduler::SwitchProcess() {
  using Status = Process::Status;
  const int base_index = current_->GetSchedulerIndex();
  for (int i = 1; i < number_of_process_; i++) {
    Process* proc = process_[(base_index + i) % number_of_process_];
    if (!proc)
      continue;
    if (proc->GetStatus() == Status::kSleeping) {
      if (current_->GetStatus() == Status::kRunning)
        current_->SetStatus(Status::kSleeping);
      proc->SetStatus(Status::kRunning);
      current_ = proc;
      return proc;
    }
  }
  return nullptr;
}

void Scheduler::KillCurrentProcess() {
  using Status = Process::Status;
  current_->SetStatus(Status::kKilled);
}
