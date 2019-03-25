#include "liumos.h"
#include "pmem.h"

void SegmentMapping::Print() {
  PutString("vaddr:");
  PutHex64ZeroFilled(vaddr_);
  PutString(" paddr:");
  PutHex64ZeroFilled(paddr_);
  PutString(" size:");
  PutHex64(map_size_);
  PutChar('\n');
}

void SegmentMapping::AllocSegmentFromPersistentMemory(
    PersistentMemoryManager& pmem) {
  SetPhysAddr(pmem.AllocPages<uint64_t>(ByteSizeToPageSize(GetMapSize())));
}

void SegmentMapping::CopyDataFrom(SegmentMapping& from) {
  assert(map_size_ == from.map_size_);
  memcpy(reinterpret_cast<void*>(GetKernelVirtAddrForPhysAddr(paddr_)),
         reinterpret_cast<void*>(GetKernelVirtAddrForPhysAddr(from.paddr_)),
         map_size_);
};

void SegmentMapping::Map(IA_PML4& page_root, uint64_t page_attr) {
  assert(GetPhysAddr());
  CreatePageMapping(*liumos->dram_allocator, page_root, GetVirtAddr(),
                    GetPhysAddr(), GetMapSize(), kPageAttrPresent | page_attr);
}

void SegmentMapping::Flush() {
  uint8_t* buf =
      GetKernelVirtAddrForPhysAddr(reinterpret_cast<uint8_t*>(GetPhysAddr()));
  CLFlush(buf, GetMapSize());
}

void ProcessMappingInfo::Print() {
  PutString("code : ");
  code.Print();
  PutString("data : ");
  data.Print();
  PutString("stack: ");
  stack.Print();
}

void ExecutionContext::Flush() {
  map_info_.Flush();
  CLFlush(this, sizeof(*this));
}

void PersistentProcessInfo::Print() {
  if (!IsValid()) {
    PutString("Invalid Persistent Process Info\n");
    return;
  }
  PutString("Persistent Process Info:\n");
  PutString("seg_map[0]:\n");
  ctx_[0].GetProcessMappingInfo().Print();
  PutStringAndHex("RIP", ctx_[0].GetCPUContext().int_ctx.rip);
  PutString("seg_map[1]:\n");
  ctx_[1].GetProcessMappingInfo().Print();
  PutStringAndHex("RIP", ctx_[1].GetCPUContext().int_ctx.rip);
  PutStringAndHex("valid_ctx_idx_", valid_ctx_idx_);
}

void PersistentProcessInfo::SwitchContext() {
  GetWorkingContext().Flush();
  SetValidContextIndex(1 - valid_ctx_idx_);
  GetWorkingContext().CopyContextFrom(GetValidContext());
}
