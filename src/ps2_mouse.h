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

  struct MouseEvent {
    bool buttonL;
    bool buttonC;
    bool buttonR;
    int8_t dx;
    int8_t dy;
  };

  static constexpr int kBufferSize = 32;
  RingBuffer<MouseEvent, kBufferSize> buffer;

 private:
  static PS2MouseController* mouse_ctrl_;
  enum Phase {
    kInitialState,
    kWaitingACKForInitCommand,
    kWaitingInitSuccess,
    kWaitingACKForSamplingRateCmd,
    kWaitingACKForSamplingRateData,
    kWaitingACKForGetDeviceIDCmd,
    kWaitingDeviceID,
    kWaitingACKForStartDataReportingCmd,
    kWaitingFirstByte,
    kWaitingSecondByte,
    kWaitingThirdByte,
  } phase_;
  uint8_t data[3];

  static void IntHandlerEntry(uint64_t intcode, InterruptInfo* info) {
    assert(mouse_ctrl_);
    mouse_ctrl_->IntHandler(intcode, info);
  }

  PS2MouseController(){};
  void IntHandler(uint64_t intcode, InterruptInfo* info);
};

void MouseManager();
