#pragma once
#include "process.h"
#include "process_lock.h"

class Scheduler {
 public:
  Scheduler(Process& root_process)
      : number_of_process_(0), current_(&root_process) {
    RegisterProcess(root_process);
    root_process.SetStatus(Process::Status::kRunning);
  }
  void RegisterProcess(Process& proc);
  uint64_t LaunchAndWaitUntilExit(Process& proc);
  Process* SwitchProcess();
  Process& GetCurrentProcess() {
    assert(current_);
    return *current_;
  }
  void KillCurrentProcess();
  Process* GetProcess(int id);
  int GetNumOfProcess();
  void Kill(Process::PID pid);

 private:
  const static int kNumberOfProcess = 256;
  Process* process_[kNumberOfProcess];
  int number_of_process_;
  Process* current_;
  ProcessLock lock_;
};
