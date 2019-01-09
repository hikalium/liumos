#pragma once
#include "execution_context.h"

class Scheduler {
 public:
  Scheduler(ExecutionContext* root_context) : number_of_contexts_(0) {
    RegisterExecutionContext(root_context);
    WriteMSR(MSRIndex::kKernelGSBase, reinterpret_cast<uint64_t>(root_context));
    SwapGS();
    root_context->SetStatus(ExecutionContext::Status::kSleeping);
  }
  void RegisterExecutionContext(ExecutionContext* context);
  ExecutionContext* SwitchContext(ExecutionContext* current_context);

 private:
  const static int kNumberOfContexts = 16;
  ExecutionContext* contexts_[kNumberOfContexts];
  int number_of_contexts_;
};
