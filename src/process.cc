#include "liumos.h"

void Process::WaitUntilExit() {
  while (status_ != Status::kKilled) {
    StoreIntFlagAndHalt();
  }
}

void Process::NotifyContextSaving() {
  number_of_ctx_switch_++;
  if (!IsPersistent())
    return;
  pp_info_->SwitchContext();
}

Process& ProcessController::Create() {
  Process* proc = kernel_heap_allocator_.AllocPages<Process*>(
      ByteSizeToPageSize(sizeof(Process)),
      kPageAttrPresent | kPageAttrWritable);
  new (proc) Process(++last_id_);
  return *proc;
}

static void PrepareContextForRestoringPersistentProcess(ExecutionContext& ctx) {
  ProcessMappingInfo& map_info = ctx.GetProcessMappingInfo();

  IA_PML4& pt = AllocPageTable();
  SetKernelPageEntries(pt);
  map_info.code.Map(pt, kPageAttrUser);
  map_info.data.Map(pt, kPageAttrUser | kPageAttrWritable);
  map_info.stack.Map(pt, kPageAttrUser | kPageAttrWritable);
  ctx.SetCR3(reinterpret_cast<uint64_t>(&pt));
  ctx.SetKernelRSP(liumos->kernel_heap_allocator->AllocPages<uint64_t>(
                       kKernelStackPagesForEachProcess,
                       kPageAttrPresent | kPageAttrWritable) +
                   (kKernelStackPagesForEachProcess << kPageSizeExponent));
}

Process& ProcessController::RestoreFromPersistentProcessInfo(
    PersistentProcessInfo& pp_info_in_paddr) {
  PersistentProcessInfo& pp_info =
      *GetKernelVirtAddrForPhysAddr(&pp_info_in_paddr);

  ExecutionContext& valid_ctx = pp_info.GetValidContext();
  ExecutionContext& working_ctx = pp_info.GetWorkingContext();

  working_ctx.CopyContextFrom(valid_ctx);

  PrepareContextForRestoringPersistentProcess(valid_ctx);
  PrepareContextForRestoringPersistentProcess(working_ctx);

  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsPersistentProcess(pp_info);

  return proc;
}
