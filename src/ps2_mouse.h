#pragma once

#include "liumos.h"

class PS2MouseController {
 public:
  void Init();

  static PS2MouseController& GetInstance() {
    if (!mouse_ctrl_)
      mouse_ctrl_ = new PS2MouseController();
    assert(mouse_ctrl_);
    return *mouse_ctrl_;
  }

 private:
  static PS2MouseController* mouse_ctrl_;

  static void IntHandlerEntry(uint64_t intcode, InterruptInfo* info) {
    assert(mouse_ctrl_);
    mouse_ctrl_->IntHandler(intcode, info);
  }

  PS2MouseController(){};
  void IntHandler(uint64_t intcode, InterruptInfo* info);
};
