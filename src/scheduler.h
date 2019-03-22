#pragma once
#include "process.h"

class Scheduler {
 public:
  Scheduler(Process& root_process)
      : number_of_process_(0), current_(&root_process) {
    RegisterProcess(root_process);
    root_process.SetStatus(Process::Status::kRunning);
  }
  void RegisterProcess(Process& proc);
  Process* SwitchProcess();
  Process& GetCurrentProcess() {
    assert(current_);
    return *current_;
  }
  void KillCurrentProcess();

 private:
  const static int kNumberOfProcess = 16;
  Process* process_[kNumberOfProcess];
  int number_of_process_;
  Process* current_;
};
