#include "scheduler.h"

#include "liumos.h"

void Scheduler::RegisterProcess(Process& proc) {
  using Status = Process::Status;
  lock_.Lock();
  assert(number_of_process_ < kNumberOfProcess);
  assert(proc.GetStatus() == Process::Status::kNotScheduled);
  for (int i = 0; i < number_of_process_; i++) {
    if (process_[i]->GetStatus() != Status::kStopped)
      continue;
    process_[i] = &proc;
    proc.SetSchedulerIndex(i);
    proc.SetStatus(Status::kSleeping);
    lock_.Unlock();
    return;
  }
  process_[number_of_process_] = &proc;
  proc.SetSchedulerIndex(number_of_process_);
  number_of_process_++;

  proc.SetStatus(Status::kSleeping);
  lock_.Unlock();
}

uint64_t Scheduler::LaunchAndWaitUntilExit(Process& proc) {
  RegisterProcess(proc);
  proc.WaitUntilExit();
  proc.PrintStatistics();
  return 0;
}

Process* Scheduler::SwitchProcess() {
  using Status = Process::Status;
  lock_.Lock();
  const int base_index = current_->GetSchedulerIndex();
  for (int i = 1; i < number_of_process_; i++) {
    Process* proc = process_[(base_index + i) % number_of_process_];
    if (!proc)
      continue;
    if (proc->GetStatus() == Status::kSleeping) {
      if (current_->GetStatus() == Status::kRunning)
        current_->SetStatus(Status::kSleeping);
      else if (current_->GetStatus() == Status::kStopping)
        current_->SetStatus(Status::kStopped);
      proc->SetStatus(Status::kRunning);
      current_ = proc;
      lock_.Unlock();
      return proc;
    }
  }
  lock_.Unlock();
  return nullptr;
}

void Scheduler::KillCurrentProcess() {
  current_->Kill();
}

Process* Scheduler::GetProcess(int id) {
  if (id >= number_of_process_)
    return NULL;
  return process_[id];
}

int Scheduler::GetNumOfProcess() {
  return number_of_process_;
}

void Scheduler::Kill(Process::PID pid) {
  lock_.Lock();
  for (int i = 0; i < number_of_process_; i++) {
    Process* proc = process_[i];
    if (proc->GetID() == pid) {
      proc->Kill();
      lock_.Unlock();
      return;
    }
  }
  lock_.Unlock();
}
