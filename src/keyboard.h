#pragma once

#include "generic.h"
#include "ring_buffer.h"

class KeyboardController {
 public:
  void Init();
  static void IntHandler(uint64_t intcode,
                         uint64_t error_code,
                         InterruptInfo* info) {
    last_instance_->IntHandlerSub(intcode, error_code, info);
  }
  uint16_t ReadKeyID() {
    while (!keycode_buffer_.IsEmpty()) {
      uint16_t keyid = ParseKeyCode(keycode_buffer_.Pop());
      if (keyid)
        return keyid;
    }
    return 0;
  }

 private:
  void IntHandlerSub(uint64_t intcode,
                     uint64_t error_code,
                     InterruptInfo* info);
  uint16_t ParseKeyCode(uint8_t keycode);
  static KeyboardController* last_instance_;
  RingBuffer<uint8_t, 16> keycode_buffer_;
  bool state_shift_;
};

uint16_t ParseKeyCode(uint8_t keycode);
