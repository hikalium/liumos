#include "liumos.h"

void SegmentMapping::Print() {
  PutString("vaddr:");
  PutHex64ZeroFilled(vaddr);
  PutString(" paddr:");
  PutHex64ZeroFilled(paddr);
  PutString(" size:");
  PutHex64(size);
  PutChar('\n');
}

void ProcessMappingInfo::Print() {
  PutString("code: ");
  code.Print();
  PutString("data: ");
  data.Print();
  PutString("stack: ");
  stack.Print();
}

ExecutionContext& ExecutionContextController::Create(void (*rip)(),
                                                     uint16_t cs,
                                                     void* rsp,
                                                     uint16_t ss,
                                                     uint64_t cr3,
                                                     uint64_t rflags,
                                                     uint64_t kernel_rsp) {
  ExecutionContext& context =
      *kernel_heap_allocator_.AllocPages<ExecutionContext*>(
          ByteSizeToPageSize(sizeof(ExecutionContext)),
          kPageAttrPresent | kPageAttrWritable);
  new (&context) ExecutionContext(rip, cs, rsp, ss, cr3, rflags, kernel_rsp);
  return context;
}
