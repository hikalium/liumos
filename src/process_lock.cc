#include "process_lock.h"
#include "liumos.h"

void ProcessLock::Lock() {
  if (!liumos->is_multi_task_enabled)
    return;
  Process& proc = liumos->scheduler->GetCurrentProcess();
  uint64_t pid = proc.GetID();
  CompareAndExchange64(&locked_by_pid_, 0, pid);
}

void ProcessLock::Unlock() {
  if (!liumos->is_multi_task_enabled)
    return;
  locked_by_pid_ = 0;
}
