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

static void PrepareContextForRestoringPersistentProcess(ExecutionContext& ctx) {
  ProcessMappingInfo& map_info = ctx.GetProcessMappingInfo();

  IA_PML4& pt = CreatePageTable();
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
      *liumos->kernel_heap_allocator->MapPages<PersistentProcessInfo*>(
          reinterpret_cast<uint64_t>(&pp_info_in_paddr),
          ByteSizeToPageSize(sizeof(PersistentProcessInfo)),
          kPageAttrPresent | kPageAttrWritable);

  ExecutionContext& valid_ctx = pp_info.GetValidContext();
  ExecutionContext& working_ctx = pp_info.GetWorkingContext();
  ProcessMappingInfo& valid_map_info = valid_ctx.GetProcessMappingInfo();
  ProcessMappingInfo& working_map_info = working_ctx.GetProcessMappingInfo();

  working_ctx.GetCPUContext() = valid_ctx.GetCPUContext();
  working_map_info.data.CopyDataFrom(valid_map_info.data);
  working_map_info.stack.CopyDataFrom(valid_map_info.stack);

  PrepareContextForRestoringPersistentProcess(valid_ctx);
  PrepareContextForRestoringPersistentProcess(working_ctx);

  Process& proc = liumos->proc_ctrl->Create();
  proc.InitAsPersistentProcess(pp_info);

  return proc;
}
