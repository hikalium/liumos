#include "liumos.h"

void ExecutionContext::WaitUntilExit() {
  while (status_ != Status::kKilled) {
    StoreIntFlagAndHalt();
  }
}

ExecutionContext* CreateExecutionContext(void (*rip)(),
                                         uint16_t cs,
                                         void* rsp,
                                         uint16_t ss,
                                         uint64_t cr3) {
  static uint64_t context_id;

  ExecutionContext* context = dram_allocator->AllocPages<ExecutionContext*>(
      ByteSizeToPageSize(sizeof(ExecutionContext)));
  new (context) ExecutionContext(++context_id, rip, cs, rsp, ss, cr3);
  return context;
}
