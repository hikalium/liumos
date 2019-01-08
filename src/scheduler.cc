#include "scheduler.h"
#include "liumos.h"

void Scheduler::RegisterExecutionContext(ExecutionContext* context) {
  assert(number_of_contexts_ < kNumberOfContexts);
  assert(context->GetStatus() == ExecutionContext::Status::kNotScheduled);
  contexts_[number_of_contexts_] = context;
  context->SetSchedulerIndex(number_of_contexts_);
  number_of_contexts_++;

  context->SetStatus(ExecutionContext::Status::kSleeping);
}

ExecutionContext* Scheduler::SwitchContext(ExecutionContext* current_context) {
  const int base_index = current_context->GetSchedulerIndex();
  for (int i = 1; i < number_of_contexts_; i++) {
    ExecutionContext* context =
        contexts_[(base_index + i) % number_of_contexts_];
    if (context->GetStatus() == ExecutionContext::Status::kSleeping) {
      current_context->SetStatus(ExecutionContext::Status::kSleeping);
      context->SetStatus(ExecutionContext::Status::kRunning);
      return context;
    }
  }
  return nullptr;
}
