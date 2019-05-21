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

void SegmentMapping::CopyDataFrom(SegmentMapping& from,
                                  uint64_t& stat_copied_bytes) {
  assert(map_size_ == from.map_size_);
  memcpy(reinterpret_cast<void*>(GetKernelVirtAddrForPhysAddr(paddr_)),
         reinterpret_cast<void*>(GetKernelVirtAddrForPhysAddr(from.paddr_)),
         map_size_);
  stat_copied_bytes += map_size_;
};

void SegmentMapping::Flush(IA_PML4& pml4, uint64_t& num_of_clflush_issued) {
  FlushDirtyPages(pml4, vaddr_, map_size_, num_of_clflush_issued);
}

void ProcessMappingInfo::Print() {
  PutString("code : ");
  code.Print();
  PutString("data : ");
  data.Print();
  PutString("stack: ");
  stack.Print();
}

void ExecutionContext::ExpandHeap(int64_t diff) {
  uint64_t diff_abs = diff < 0 ? -diff : diff;
  if (diff_abs > map_info_.heap.GetMapSize()) {
    PutStringAndDecimal("diff_abs", diff_abs);
    PutStringAndDecimal("map_size", map_info_.heap.GetMapSize());
    Panic("Too large heap expansion request");
  }
  heap_used_size_ += diff;
  if (heap_used_size_ > map_info_.heap.GetMapSize())
    Panic("No more heap");
};

void ExecutionContext::Flush(IA_PML4& pml4, uint64_t& num_of_clflush_issued) {
  map_info_.Flush(pml4, num_of_clflush_issued);
  CLFlush(this, sizeof(*this), num_of_clflush_issued);
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

void PersistentProcessInfo::SwitchContext(uint64_t& stat_copied_bytes,
                                          uint64_t& stat_num_of_clflush) {
  GetWorkingContext().Flush(GetWorkingContext().GetCR3(), stat_num_of_clflush);
  SetValidContextIndex(1 - valid_ctx_idx_);
  GetWorkingContext().CopyContextFrom(GetValidContext(), stat_copied_bytes);
}
