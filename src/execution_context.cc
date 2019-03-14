#include "liumos.h"

void ExecutionContext::WaitUntilExit() {
  while (status_ != Status::kKilled) {
    StoreIntFlagAndHalt();
  }
}

ExecutionContext* ExecutionContextController::Create(void (*rip)(),
                                                     uint16_t cs,
                                                     void* rsp,
                                                     uint16_t ss,
                                                     uint64_t cr3) {
  ExecutionContext* context =
      kernel_heap_allocator_.AllocPages<ExecutionContext*>(
          ByteSizeToPageSize(sizeof(ExecutionContext)),
          kPageAttrPresent | kPageAttrWritable);
  new (context) ExecutionContext(++last_context_id_, rip, cs, rsp, ss, cr3);
  return context;
}
