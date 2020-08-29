#include "ps2_mouse.h"

#include "liumos.h"

PS2MouseController* PS2MouseController::mouse_ctrl_;

constexpr uint16_t kIOPortKeyboardStatus = 0x0064;
constexpr uint16_t kIOPortKeyboardCmd = 0x0064;
constexpr uint8_t kKBCStatusNotReady = 0x02;
constexpr uint8_t kKBCCmdPrefixSendToMouse = 0xD4;
constexpr uint8_t kKBCCmdMouseReset = 0xFF;

static void WaitForReadyToSendKBCCmd(void) {
  while (ReadIOPort8(kIOPortKeyboardStatus) & kKBCStatusNotReady) {
    asm volatile("pause;");
  }
}

static void SendToMouse(uint8_t v) {
  WaitForReadyToSendKBCCmd();
  WriteIOPort8(kIOPortKeyboardCmd, kKBCCmdPrefixSendToMouse);
  WaitForReadyToSendKBCCmd();
  WriteIOPort8(kIOPortKeyboardData, v);
}

void PS2MouseController::Init() {
  PutString("Initializing PS/2 Mouse...");
  liumos->idt->SetIntHandler(0x22, PS2MouseController::IntHandlerEntry);
  SendToMouse(kKBCCmdMouseReset);
  PutString("done.\n");
}

void PS2MouseController::IntHandler(uint64_t, InterruptInfo*) {
  PutStringAndHex("Mouse", ReadIOPort8(kIOPortKeyboardData));
  liumos->bsp_local_apic->SendEndOfInterrupt();
}
