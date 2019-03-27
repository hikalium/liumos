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

void Scheduler::LaunchAndWaitUntilExit(Process& proc) {
  liumos->main_console->SetSerial(nullptr);
  liumos->root_process->ResetProcTimeFemtoSec();
  liumos->sub_process->ResetProcTimeFemtoSec();
  uint64_t t0 = liumos->hpet->ReadMainCounterValue();
  RegisterProcess(proc);
  proc.WaitUntilExit();
  uint64_t t1 = liumos->hpet->ReadMainCounterValue();
  liumos->main_console->SetSerial(liumos->com1);
  PutStringAndHex("Nano Second",
                  (t1 - t0) * liumos->hpet->GetFemtosecndPerCount() / 1000'000);
  PutStringAndHex("NumOfCtxSw", proc.GetNumberOfContextSwitch());
  PutStringAndHex("TimeInTask(ns)         ",
                  proc.GetProcTimeFemtoSec() / 1000'000);
  PutStringAndHex("SysTimeInTask(ns)      ",
                  proc.GetSysTimeFemtoSec() / 1000'000);
  PutStringAndHex("TimeInTask(ns) roottask",
                  liumos->root_process->GetProcTimeFemtoSec() / 1000'000);
  PutStringAndHex("TimeInTask(ns) subtask ",
                  liumos->sub_process->GetProcTimeFemtoSec() / 1000'000);
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
