#include "ps2_mouse.h"
#include "kernel.h"
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
  IDT::GetInstance().SetIntHandler(0x22, PS2MouseController::IntHandlerEntry);
  SendToMouse(kKBCCmdMouseReset);
  phase_ = kWaitingACKForInitCommand;
  PutString("done.\n");
}

void PS2MouseController::IntHandler(uint64_t, InterruptInfo*) {
  constexpr uint8_t kFirstByteBitButtonL = 0b0000001;
  constexpr uint8_t kFirstByteBitButtonR = 0b0000010;
  constexpr uint8_t kFirstByteBitButtonC = 0b0000100;

  uint8_t value = ReadIOPort8(kIOPortKeyboardData);
  switch (phase_) {
    case kWaitingACKForInitCommand:
      if (value == 0xFA /* ACK */) {
        phase_ = kWaitingInitSuccess;
      }
      break;
    case kWaitingInitSuccess:
      if (value == 0xAA /* TEST SUCCEEDED */) {
        SendToMouse(0xF3);
        phase_ = kWaitingACKForSamplingRateCmd;
      }
      break;
    case kWaitingACKForSamplingRateCmd:
      if (value == 0xFA /* ACK */) {
        SendToMouse(100);
        phase_ = kWaitingACKForSamplingRateData;
      }
      break;
    case kWaitingACKForSamplingRateData:
      if (value == 0xFA /* ACK */) {
        SendToMouse(0xF2);
        phase_ = kWaitingACKForGetDeviceIDCmd;
      }
      break;
    case kWaitingACKForGetDeviceIDCmd:
      if (value == 0xFA /* ACK */) {
        phase_ = kWaitingDeviceID;
      }
      break;
    case kWaitingDeviceID:
      // value == 0x00 : 3 button mouse
      // value == 0x03 : 3 button mouse with scroll
      SendToMouse(0xF4 /* Start data reporting */);
      phase_ = kWaitingACKForStartDataReportingCmd;
      break;
    case kWaitingACKForStartDataReportingCmd:
      if (value == 0xFA /* ACK */) {
        kprintf("PS/2 Mouse connected\n");
        phase_ = kWaitingFirstByte;
      }
      break;
    case kWaitingFirstByte:
      // Expected first byte bits: 00XY1CRL
      data[0] = value;
      if ((data[0] & 0b11001000) == 0b00001000) {
        phase_ = kWaitingSecondByte;
      }
      break;
    case kWaitingSecondByte:
      data[1] = value;
      phase_ = kWaitingThirdByte;
      break;
    case kWaitingThirdByte: {
      data[2] = value;
      MouseEvent me;
      me.buttonL = data[0] & kFirstByteBitButtonL;
      me.buttonR = data[0] & kFirstByteBitButtonR;
      me.buttonC = data[0] & kFirstByteBitButtonC;
      me.dx = static_cast<int8_t>(data[1]);
      me.dy = -static_cast<int8_t>(data[2]);
      buffer.Push(me);
      phase_ = kWaitingFirstByte;
    } break;
    default:
      break;
  }
  liumos->bsp_local_apic->SendEndOfInterrupt();
}

static void FixPositionInVRAM(int& px, int& py) {
  assert(liumos->vram_sheet);
  Sheet vram = *liumos->vram_sheet;
  if (px < 0)
    px = 0;
  if (py < 0)
    py = 0;
  if (px >= vram.GetXSize())
    px = vram.GetXSize() - 1;
  if (py >= vram.GetYSize())
    py = vram.GetYSize() - 1;
}

constexpr int kMouseCursorSize = 10;

static void DrawMouseCursor(int& px, int& py) {
  FixPositionInVRAM(px, py);
  assert(liumos->vram_sheet);
  Sheet vram = *liumos->vram_sheet;
  Rect mrect = vram.GetRect().GetIntersectionWith(
      {px, py, kMouseCursorSize, kMouseCursorSize});
  for (int x = 0; x < mrect.xsize; x++) {
    SheetPainter::DrawPoint(vram, px + x, py, 0x00FF00, false);
  }
  for (int y = 0; y < mrect.ysize; y++) {
    SheetPainter::DrawPoint(vram, px, py + y, 0x00FF00, false);
  }
}

static void MoveMouseCursor(int& px, int& py, int dx, int dy) {
  // Erase cursor
  liumos->screen_sheet->Flush(px, py, kMouseCursorSize, kMouseCursorSize);
  // Move and redraw
  px += dx;
  py += dy;
  DrawMouseCursor(px, py);
}

void MouseManager() {
  auto& mctrl = PS2MouseController::GetInstance();
  int mx = 50, my = 50;
  DrawMouseCursor(mx, my);
  for (;;) {
    if (mctrl.buffer.IsEmpty()) {
      Sleep();
      continue;
    }
    auto me = mctrl.buffer.Pop();
    MoveMouseCursor(mx, my, me.dx, me.dy);
  }
}
