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
  pp_info_->SwitchContext(copied_bytes_in_ctx_sw_);
}

void Process::PrintStatistics() {
  PutStringAndDecimal("Process id", id_);
  PutStringAndDecimal("  number_of_ctx_switch_", number_of_ctx_switch_);
  PutStringAndDecimalWithPointPos("  proc_time (sec)", proc_time_femto_sec_,
                                  15);
  PutStringAndDecimalWithPointPos("  sys_time  (sec)", sys_time_femto_sec_, 15);
  PutStringAndDecimalWithPointPos("  copied_bytes_in_ctx_sw  (MB)",
                                  copied_bytes_in_ctx_sw_, 6);
}

Process& ProcessController::Create() {
  Process* proc = kernel_heap_allocator_.AllocPages<Process*>(
      ByteSizeToPageSize(sizeof(Process)),
      kPageAttrPresent | kPageAttrWritable);
  new (proc) Process(++last_id_);
  return *proc;
}

static void PrepareContextForRestoringPersistentProcess(ExecutionContext& ctx) {
  SetKernelPageEntries(ctx.GetCR3());
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

  uint64_t dummy_stat;
  working_ctx.CopyContextFrom(valid_ctx, dummy_stat);

  PrepareContextForRestoringPersistentProcess(valid_ctx);
  PrepareContextForRestoringPersistentProcess(working_ctx);

  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsPersistentProcess(pp_info);

  return proc;
}
