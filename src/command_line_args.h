#pragma once

#include "generic.h"

class CommandLineArgs {
 public:
  CommandLineArgs() : argc_(0) {}
  int GetNumOfArgs() { return argc_; }
  const char* GetArg(int index) {
    if (index < 0 || argc_ <= index)
      return nullptr;
    return argv_[index];
  }
  bool Parse(const char* const s) {
    // Returns s is completely parsed or not
    int buf_used = 0;
    argc_ = 0;
    for (int i = 0;;) {
      while (s[i] == ' ')
        i++;
      if (!s[i])
        return true;
      if (argc_ >= kMaxNumOfArgs)
        return false;
      argv_[argc_++] = &buf_[buf_used];
      while (s[i] && s[i] != ' ') {
        buf_[buf_used++] = s[i++];
        if (buf_used >= kBufSize)
          return false;
      }
      buf_[buf_used++] = 0;
    }
    return false;
  }

  static constexpr int kMaxNumOfArgs = 16;
  static constexpr int kBufSize = 64;

 private:
  int argc_;
  const char* argv_[kMaxNumOfArgs];
  char buf_[kBufSize];
};
