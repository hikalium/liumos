#pragma once

#include "generic.h"

class ProcessLock {
 public:
  ProcessLock() : locked_by_pid_(0) {}
  void Lock();
  void Unlock();

 private:
  uint64_t locked_by_pid_;
};
