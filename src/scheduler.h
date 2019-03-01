#pragma once
#include "execution_context.h"

class Scheduler {
 public:
  Scheduler(ExecutionContext* root_context)
      : number_of_contexts_(0), current_(root_context) {
    RegisterExecutionContext(root_context);
    root_context->SetStatus(ExecutionContext::Status::kRunning);
  }
  void RegisterExecutionContext(ExecutionContext* context);
  ExecutionContext* SwitchContext();
  ExecutionContext* GetCurrentContext() { return current_; }

 private:
  const static int kNumberOfContexts = 16;
  ExecutionContext* contexts_[kNumberOfContexts];
  int number_of_contexts_;
  ExecutionContext* current_;
};
