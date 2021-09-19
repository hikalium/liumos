#pragma once
#include "generic.h"

class TextBox {
 public:
  TextBox() : buf_used_(0), is_recording_enabled_(false) {}
  void putc(uint16_t keyid);
  void StartRecording() {
    buf_used_ = 0;
    buf_[buf_used_] = 0;
    is_recording_enabled_ = true;
  }
  void StopRecording() { is_recording_enabled_ = false; }
  const char* GetRecordedString() { return buf_; }
  constexpr static int kSizeOfBuffer = 128;

 private:
  char buf_[kSizeOfBuffer + 1];
  int buf_used_;
  bool is_recording_enabled_;
};
