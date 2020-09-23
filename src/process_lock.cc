#include "process_lock.h"
#include "liumos.h"
#include "panic_printer.h"

void ProcessLock::Lock() {
  if (!liumos->is_multi_task_enabled)
    return;
#ifndef LIUMOS_LOADER
  Process& proc = liumos->scheduler->GetCurrentProcess();
  uint64_t pid = proc.GetID();
  for (int cnt = 0; cnt < 3; cnt++) {
    if (!CompareAndExchange64(&locked_by_pid_, 0, pid))
      return;
  }
  auto& pp = PanicPrinter::BeginPanic();
  StringBuffer<128> buf;

  buf.WriteString("pid 0x");
  buf.WriteHex64(pid);
  buf.WriteString(" tried to acquire a lock");
  pp.PrintLine(buf.GetString());
  buf.Clear();

  buf.WriteString("which is locked by pid 0x");
  buf.WriteHex64(pid);
  pp.PrintLine(buf.GetString());
  buf.Clear();

  pp.EndPanicAndDie("ProcessLock::Lock() taking too long (deadlock?)");
#endif
}

void ProcessLock::Unlock() {
  if (!liumos->is_multi_task_enabled)
    return;
  locked_by_pid_ = 0;
}
