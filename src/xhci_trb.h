#pragma once

#include "util.h"

namespace XHCI {

struct BasicTRB {
  volatile uint64_t data;
  volatile uint32_t option;
  volatile uint32_t control;

  static constexpr uint32_t kTRBTypeSetupStage = 2;
  static constexpr uint32_t kTRBTypeDataStage = 3;
  static constexpr uint32_t kTRBTypeStatusStage = 4;

  static constexpr uint8_t kCompletionCodeSuccess = 0x01;
  static constexpr uint8_t kCompletionCodeUSBTransactionError = 0x04;
  static constexpr uint8_t kCompletionCodeTRBError = 0x05;
  static constexpr uint8_t kCompletionCodeShortPacket = 0x0D;

  uint8_t GetTRBType() const { return GetBits<15, 10, uint8_t>(control); }
  uint8_t GetSlotID() const { return GetBits<31, 24, uint8_t>(control); }
  uint8_t GetCompletionCode() const { return GetBits<31, 24, uint8_t>(option); }
  int GetTransferSize() const { return GetBits<23, 0, int>(option); }
  bool IsCompletedWithSuccess() {
    return GetCompletionCode() == kCompletionCodeSuccess;
  }
  bool IsCompletedWithShortPacket() {
    return GetCompletionCode() == kCompletionCodeShortPacket;
  }
#ifndef LIUMOS_TEST
  void PrintCompletionCode() {
    uint8_t code = GetCompletionCode();
    PutStringAndHex("  CompletionCode", code);
    if (code == kCompletionCodeSuccess) {
      PutString("  = Success\n");
    }
    if (code == kCompletionCodeUSBTransactionError) {
      PutString("  = USB Transaction Error\n");
    }
    if (code == kCompletionCodeTRBError) {
      PutString("  = TRB Error\n");
    }
    if (code == kCompletionCodeShortPacket) {
      PutString("  = Short Packet\n");
    }
  }
#endif
};
static_assert(sizeof(BasicTRB) == 16);

}  // namespace XHCI
