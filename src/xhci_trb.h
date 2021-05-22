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
  static constexpr uint32_t kTRBTypeEnableSlotCommand = 9;
  static constexpr uint32_t kTRBTypeAddressDeviceCommand = 11;
  static constexpr uint32_t kTRBTypeNoOpCommand = 23;
  static constexpr uint32_t kTRBTypeTransferEvent = 32;
  static constexpr uint32_t kTRBTypeCommandCompletionEvent = 33;
  static constexpr uint32_t kTRBTypePortStatusChangeEvent = 34;

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
  void PrintHex() const volatile {
    const volatile uint8_t* buf =
        reinterpret_cast<const volatile uint8_t*>(this);
    for (size_t i = 0; i < sizeof(*this); i++) {
      PutHex8ZeroFilled(buf[i]);
      if ((i & 3) == 3)
        PutChar('\n');
    }
  }
#endif
};
static_assert(sizeof(BasicTRB) == 16);

struct SetupStageTRB {
  // [xHCI] 6.4.1.2.1 Setup Stage TRB
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
  static constexpr uint8_t kReqSetInterface = 11;

  static constexpr uint32_t kControlBitImmediateData = 1 << 6;

  enum class TransferType : uint32_t {
    // Table 4-7: USB SETUP Data to Data Stage TRB and Status Stage TRB mapping
    kNoDataStage = 0,
    kOutDataStage = 2,
    kInDataStage = 3,
  };

  void SetParams(uint8_t bmRequestType,
                 uint8_t bRequest,
                 uint16_t wValue,
                 uint16_t wIndex,
                 uint16_t wLength,
                 bool shoud_interrupt_on_completion) {
    SetRequest(bmRequestType, bRequest, wValue, wIndex, wLength);
    option = 8;  // always 8
    SetControl((wLength == 0) ? TransferType::kNoDataStage
                              : ((bRequest & kReqTypeBitDirectionDeviceToHost)
                                     ? TransferType::kInDataStage
                                     : TransferType::kOutDataStage),
               shoud_interrupt_on_completion);
  }

#ifndef LIUMOS_TEST
  void Print() {
    PutString("SetupStageTRB:\n");
    PutStringAndHex("  request_type", request_type);
    PutStringAndHex("  request", request);
    PutStringAndHex("  value", value);
    PutStringAndHex("  index", index);
    PutStringAndHex("  length", length);
    PutStringAndHex("  option", option);
    PutStringAndHex("  control", control);
  }
#endif

 private:
  void SetRequest(uint8_t request_type,
                  uint8_t request,
                  uint16_t value,
                  uint16_t index,
                  uint16_t length) {
    this->request_type = request_type;
    this->request = request;
    this->value = value;
    this->index = index;
    this->length = length;
  }
  void SetControl(TransferType transfer_type,
                  bool shoud_interrupt_on_completion) {
    // Cycle bit will be set in TRBRing::Push();
    control = (static_cast<uint32_t>(transfer_type) << 16) |
              (BasicTRB::kTRBTypeSetupStage << 10) | kControlBitImmediateData |
              (static_cast<uint32_t>(shoud_interrupt_on_completion) << 5);
  }
};
static_assert(sizeof(SetupStageTRB) == 16);

struct DataStageTRB {
  volatile uint64_t buf;
  volatile uint32_t option;
  volatile uint32_t control;

  void SetControl(bool is_data_direction_in,
                  bool is_immediate_data,
                  bool shoud_interrupt_on_completion) {
    // Cycle bit will be set in TRBRing::Push();
    control =
        (static_cast<uint32_t>(is_data_direction_in) << 16) |
        (BasicTRB::kTRBTypeDataStage << 10) |
        (static_cast<uint32_t>(is_immediate_data) << 6) |
        (static_cast<uint32_t>(shoud_interrupt_on_completion) << 5 | (1 << 2));
  }
#ifndef LIUMOS_TEST
  void Print() {
    PutString("DataStageTRB:\n");
    PutStringAndHex("  buf", buf);
    PutStringAndHex("  option", option);
    PutStringAndHex("  control", control);
  }
#endif
};
static_assert(sizeof(DataStageTRB) == 16);

struct StatusStageTRB {
  volatile uint64_t reserved;
  volatile uint32_t option;
  volatile uint32_t control;

  void SetParams(bool is_status_direction_in,
                 bool shoud_interrupt_on_completion) {
    reserved = 0;
    option = 0;
    SetControl(is_status_direction_in, shoud_interrupt_on_completion);
  }

#ifndef LIUMOS_TEST
  void Print() {
    PutString("StatusStageTRB:\n");
    PutStringAndHex("  reserved", reserved);
    PutStringAndHex("  option", option);
    PutStringAndHex("  control", control);
  }
#endif

 private:
  void SetControl(bool is_status_direction_in,
                  bool shoud_interrupt_on_completion) {
    control = (static_cast<uint32_t>(is_status_direction_in) << 16) |
              (BasicTRB::kTRBTypeStatusStage << 10) |
              (static_cast<uint32_t>(shoud_interrupt_on_completion) << 5);
  }
};
static_assert(sizeof(StatusStageTRB) == 16);

}  // namespace XHCI
