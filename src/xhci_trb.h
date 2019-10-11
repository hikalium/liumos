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

  uint8_t GetTRBType() const { return GetBits<15, 10>(control); }
  uint8_t GetSlotID() const { return GetBits<31, 24>(control); }
  uint8_t GetCompletionCode() const { return GetBits<31, 24>(option); }
  int GetTransferSizeResidue() const { return GetBits<23, 0>(option); }
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

struct SetupStageTRB {
  volatile uint8_t request_type;
  volatile uint8_t request;
  volatile uint16_t value;
  volatile uint16_t index;
  volatile uint16_t length;
  volatile uint32_t option;
  volatile uint32_t control;

  static constexpr uint8_t kReqTypeBitDirectionDeviceToHost = 1 << 7;

  static constexpr uint8_t kReqTypeBitRecipientInterface = 1;

  static constexpr uint8_t kReqGetDescriptor = 6;
  static constexpr uint8_t kReqSetConfiguration = 9;

  static constexpr uint32_t kTransferTypeNoDataStage = 0;
  static constexpr uint32_t kTransferTypeInDataStage = 3;
};
static_assert(sizeof(SetupStageTRB) == 16);

struct DataStageTRB {
  volatile uint64_t buf;
  volatile uint32_t option;
  volatile uint32_t control;

  static constexpr uint32_t kDirectionIn = (1 << 16);
};
static_assert(sizeof(DataStageTRB) == 16);

struct StatusStageTRB {
  volatile uint64_t reserved;
  volatile uint32_t option;
  volatile uint32_t control;
};
static_assert(sizeof(StatusStageTRB) == 16);

}  // namespace XHCI
