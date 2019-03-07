#include "liumos.h"

void Console::PutChar(char c) {
  if (serial_port_) {
    if (c == '\n')
      serial_port_->SendChar('\r');
    serial_port_->SendChar(c);
  }
  if (!sheet_) {
    if (c == '\n') {
      EFI::ConOut::PutChar('\r');
      EFI::ConOut::PutChar('\n');
      return;
    }
    EFI::ConOut::PutChar(c);
    return;
  }
  if (c == '\n') {
    cursor_y_ += 16;
    cursor_x_ = 0;
  } else if (c == '\b') {
    cursor_x_ -= 8;
  } else {
    sheet_->DrawCharacter(c, cursor_x_, cursor_y_);
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
    sheet_->DrawRect(cursor_x_, cursor_y_, 8, 16, 0x000000);
  }
  if (cursor_y_ + 16 > sheet_->GetYSize()) {
    sheet_->BlockTransfer(0, 0, 0, 16, sheet_->GetXSize(),
                          sheet_->GetYSize() - 16);
    sheet_->DrawRect(0, cursor_y_ - 16, sheet_->GetXSize(), 16, 0x000000);
    cursor_y_ -= 16;
  }
}

void PutChar(char c) {
  liumos->main_console->PutChar(c);
}

void PutString(const char* s) {
  while (*s) {
    PutChar(*(s++));
  }
}

void Console::PutChars(const char* s, int n) {
  for (int i = 0; i < n; i++) {
    PutChar(*(s++));
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

void PutStringAndHex(const char* s, uint64_t value) {
  PutString(s);
  PutString(": 0x");
  PutHex64(value);
  PutString("\n");
}

void PutStringAndHex(const char* s, void* value) {
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
