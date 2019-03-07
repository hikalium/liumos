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
      liumos->dram_allocator->AllocPages<ExecutionContext*>(
          ByteSizeToPageSize(sizeof(ExecutionContext)));
  new (context) ExecutionContext(++last_context_id_, rip, cs, rsp, ss, cr3);
  return context;
}
