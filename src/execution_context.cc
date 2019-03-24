#include "liumos.h"

void SegmentMapping::Print() {
  PutString("vaddr:");
  PutHex64ZeroFilled(vaddr_);
  PutString(" paddr:");
  PutHex64ZeroFilled(paddr_);
  PutString(" size:");
  PutHex64(map_size_);
  PutChar('\n');
}

void ProcessMappingInfo::Print() {
  PutString("code : ");
  code.Print();
  PutString("data : ");
  data.Print();
  PutString("stack: ");
  stack.Print();
}

void PersistentProcessInfo::Print() {
  if (!IsValid()) {
    PutString("Invalid Persistent Process Info\n");
    return;
  }
  PutString("Persistent Process Info:\n");
  PutString("seg_map[0]:\n");
  pmap[0].Print();
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
