#include "corefunc.h"
#include "liumos.h"

#ifndef LIUMOS_LOADER
#include "xhci.h"
#endif

void Console::PutCharWithoutLocking(char c) {
  if (serial_port_) {
    if (c == '\n')
      serial_port_->SendChar('\r');
    serial_port_->SendChar(c);
  }
  if (!sheet_) {
    if (c == '\n') {
      CoreFunc::GetEFI().PutWChar('\r');
      CoreFunc::GetEFI().PutWChar('\n');
      return;
    }
    CoreFunc::GetEFI().PutWChar(c);
    return;
  }
  if (c == '\n') {
    cursor_y_ += 16;
    cursor_x_ = 0;
  } else if (c == '\b') {
    cursor_x_ -= 8;
  } else {
    SheetPainter::DrawCharacter(*sheet_, c, cursor_x_, cursor_y_, true);
    cursor_x_ += 8;
  }
  if (cursor_x_ >= sheet_->GetXSize()) {
    cursor_y_ += 16;
    cursor_x_ = 0;
  } else if (cursor_x_ < 0) {
    cursor_y_ -= 16;
    cursor_x_ = (sheet_->GetXSize() - 8) & ~7;
  }
  if (c == '\b') {
    SheetPainter::DrawRect(*sheet_, cursor_x_, cursor_y_, 8, 16, 0x000000,
                           true);
  }
  if (cursor_y_ + 16 > sheet_->GetYSize()) {
    sheet_->BlockTransfer(0, 0, 0, 16, sheet_->GetXSize(),
                          sheet_->GetYSize() - 16);
    SheetPainter::DrawRect(*sheet_, 0, cursor_y_ - 16, sheet_->GetXSize(), 16,
                           0x000000, true);
    cursor_y_ -= 16;
  }
}

void Console::PutChar(char c) {
  lock_.Lock();
  PutCharWithoutLocking(c);
  lock_.Unlock();
}

void Console::PutString(const char* s) {
  lock_.Lock();
  while (*s) {
    PutCharWithoutLocking(*(s++));
  }
  lock_.Unlock();
}

#ifndef LIUMOS_LOADER

uint16_t Console::GetCharWithoutBlocking() {
  while (1) {
    uint16_t keyid;
    if ((keyid = liumos->keyboard_ctrl->ReadKeyID()) ||
        (keyid = XHCI::Controller::GetInstance().ReadKeyboardInput())) {
      if (!keyid && (keyid & KeyID::kMaskBreak))
        continue;
      if (keyid == KeyID::kEnter) {
        return '\n';
      }
    } else if (serial_port_ && serial_port_->IsReceived()) {
      keyid = serial_port_->ReadCharReceived();
      if (keyid == '\n') {
        continue;
      }
      if (keyid == '\r') {
        return '\n';
      }
      if (keyid == 0x7f) {
        return KeyID::kBackspace;
      }
    } else {
      break;
    }
    return keyid;
  }
  return KeyID::kNoInput;
}

#endif

void PutChar(char c) {
  CoreFunc::PutChar(c);
}

void PutString(const char* s) {
  CoreFunc::PutString(s);
}

void PutDecimal64(uint64_t value) {
  char s[20];
  int i = 0;
  for (; i < 20; i++) {
    s[i] = value % 10;
    value /= 10;
    if (!value)
      break;
  }
  for (; i >= 0; i--) {
    PutChar('0' + s[i]);
  }
}

void PutDecimal64WithPointPos(uint64_t value, int point_pos) {
  // value = 1, point_pos = 1 -> 0.1
  // value 1234, point_pos = 2 -> 12.34
  char s[20];
  int i = 0;
  for (; i < point_pos; i++) {
    s[i] = value % 10;
    value /= 10;
  }
  for (; i < 20; i++) {
    s[i] = value % 10;
    value /= 10;
    if (!value)
      break;
  }
  for (; i >= 0; i--) {
    PutChar('0' + s[i]);
    if (i == point_pos)
      PutChar('.');
  }
}

void PutHex64(uint64_t value) {
  int i;
  char s[2];
  s[1] = 0;
  for (i = 15; i > 0; i--) {
    if ((value >> (4 * i)) & 0xF)
      break;
  }
  for (; i >= 0; i--) {
    s[0] = (value >> (4 * i)) & 0xF;
    if (s[0] < 10)
      s[0] += '0';
    else
      s[0] += 'A' - 10;
    PutString(s);
  }
}

void PutHex64ZeroFilled(uint64_t value) {
  int i;
  char s[2];
  s[1] = 0;
  for (i = 15; i >= 0; i--) {
    if (i == 11 || i == 7 || i == 3)
      PutString("'");
    s[0] = (value >> (4 * i)) & 0xF;
    if (s[0] < 10)
      s[0] += '0';
    else
      s[0] += 'A' - 10;
    PutString(s);
  }
}
void PutHex64ZeroFilled(void* value) {
  PutHex64ZeroFilled(reinterpret_cast<uint64_t>(value));
}

void PutHex8ZeroFilled(uint8_t value) {
  int i;
  char s[2];
  s[1] = 0;
  for (i = 1; i >= 0; i--) {
    s[0] = (value >> (4 * i)) & 0xF;
    if (s[0] < 10)
      s[0] += '0';
    else
      s[0] += 'A' - 10;
    PutString(s);
  }
}

void PutStringAndDecimal(const char* s, uint64_t value) {
  PutString(s);
  PutString(": ");
  PutDecimal64(value);
  PutString("\n");
}

void PutStringAndDecimalWithPointPos(const char* s, uint64_t value, int pos) {
  PutString(s);
  PutString(": ");
  PutDecimal64WithPointPos(value, pos);
  PutString("\n");
}

void PutStringAndHex(const char* s, uint64_t value) {
  PutString(s);
  PutString(": 0x");
  PutHex64(value);
  PutString("\n");
}

void PutStringAndHex(const char* s, const void* value) {
  PutStringAndHex(s, reinterpret_cast<uint64_t>(value));
}

void PutStringAndBool(const char* s, bool cond) {
  PutString(s);
  PutString(": ");
  PutString(cond ? "True" : "False");
  PutString("\n");
}

void PutAddressRange(uint64_t addr, uint64_t size) {
  PutChar('[');
  PutHex64ZeroFilled(addr);
  PutChar('-');
  PutHex64ZeroFilled(addr + size);
  PutChar(')');
}
void PutAddressRange(void* addr, uint64_t size) {
  PutAddressRange(reinterpret_cast<uint64_t>(addr), size);
}
