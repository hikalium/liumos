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

static void MoveMouseCursor(int& px, int& py, int dx, int dy) {
  // Move and redraw
  px += dx;
  py += dy;
  FixPositionInVRAM(px, py);
}

static Sheet* FindWindowAtPosition(int px, int py) {
  Sheet* focused = nullptr;
  for (Sheet* s = liumos->vram_sheet->GetChildAtBottom(); s && !s->IsTopmost();
       s = s->GetUpper()) {
    if (!s->GetRect().IsPointInRect(px, py) || s->IsLocked()) {
      continue;
    }
    focused = s;
  }
  return focused;
}

void MouseManager() {
  auto& mctrl = PS2MouseController::GetInstance();
  int mx = 50, my = 50;

  constexpr int cursor_size = 8;
  Sheet* cursor_sheet = AllocKernelMemory<Sheet*>(sizeof(Sheet));
  bzero(cursor_sheet, sizeof(Sheet));
  uint32_t* cursor_buf =
      AllocKernelMemory<uint32_t*>(4 * cursor_size * cursor_size);
  for (int y = 0; y < cursor_size; y++) {
    for (int x = 0; x < cursor_size; x++) {
      cursor_buf[y * cursor_size + x] =
          x < (cursor_size - y) ? 0xff88ff88 : 0x00000000 /* transparent */;
    }
  }
  cursor_sheet->Init(cursor_buf, cursor_size, cursor_size, cursor_size, mx, my);
  cursor_sheet->SetParent(liumos->vram_sheet);
  cursor_sheet->SetTopmost(true);
  cursor_sheet->SetAlphaEnabled(true);

  constexpr int debug_info_width = 128;
  constexpr int debug_info_height = 128;
  Sheet* debug_info_sheet = AllocKernelMemory<Sheet*>(sizeof(Sheet));
  bzero(debug_info_sheet, sizeof(Sheet));
  uint32_t* debug_info_buf =
      AllocKernelMemory<uint32_t*>(4 * debug_info_width * debug_info_height);
  for (int y = 0; y < debug_info_height; y++) {
    for (int x = 0; x < debug_info_width; x++) {
      debug_info_buf[y * debug_info_width + x] = 0xffffff;
    }
  }
  debug_info_sheet->Init(debug_info_buf, debug_info_width, debug_info_height,
                         debug_info_width, 0, 0);
  debug_info_sheet->SetParent(liumos->vram_sheet);
  SheetPainter::DrawString(*debug_info_sheet, "mouse:", 8, 16, false);
  debug_info_sheet->Flush();

  Sheet* debug_info_sheet2 = AllocKernelMemory<Sheet*>(sizeof(Sheet));
  bzero(debug_info_sheet2, sizeof(Sheet));
  uint32_t* debug_info_buf2 =
      AllocKernelMemory<uint32_t*>(4 * debug_info_width * debug_info_height);
  for (int y = 0; y < debug_info_height; y++) {
    for (int x = 0; x < debug_info_width; x++) {
      debug_info_buf2[y * debug_info_width + x] = 0xffffff;
    }
  }
  debug_info_sheet2->Init(debug_info_buf2, debug_info_width, debug_info_height,
                          debug_info_width, 0, 0);
  debug_info_sheet2->SetParent(liumos->vram_sheet);
  SheetPainter::DrawString(*debug_info_sheet2, "sheet2", 8, 16, false);
  debug_info_sheet2->Flush();

  bool last_left_button_state = false;
  Sheet* focused = nullptr;
  int focused_ofs_x = 0, focused_ofs_y = 0;

  for (;;) {
    if (mctrl.buffer.IsEmpty()) {
      Sleep();
      continue;
    }
    auto me = mctrl.buffer.Pop();
    MoveMouseCursor(mx, my, me.dx, me.dy);
    cursor_sheet->SetPosition(mx, my);

    if (me.buttonL) {
      if (!last_left_button_state) {
        focused = FindWindowAtPosition(mx, my);
        if (focused) {
          focused_ofs_x = mx - focused->GetX();
          focused_ofs_y = my - focused->GetY();
        }
      }
      if (focused) {
        focused->SetPosition(mx - focused_ofs_x, my - focused_ofs_y);
      }
    } else {
      focused = nullptr;
    }
    last_left_button_state = me.buttonL;

    SheetPainter::DrawCharacter(*debug_info_sheet, me.buttonL ? 'L' : 'l',
                                8 * 2, 32, false);
    SheetPainter::DrawCharacter(*debug_info_sheet, me.buttonC ? 'C' : 'c',
                                8 * 3, 32, false);
    SheetPainter::DrawCharacter(*debug_info_sheet, me.buttonR ? 'R' : 'r',
                                8 * 4, 32, false);
    debug_info_sheet->Flush();
  }
}
