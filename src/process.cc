#include "liumos.h"

void Process::WaitUntilExit() {
  while (status_ != Status::kKilled) {
    StoreIntFlagAndHalt();
  }
}

Process& ProcessController::Create() {
  Process* proc = kernel_heap_allocator_.AllocPages<Process*>(
      ByteSizeToPageSize(sizeof(Process)),
      kPageAttrPresent | kPageAttrWritable);
  new (proc) Process(++last_id_);
  return *proc;
}
