#include "liumos.h"

extern uint8_t* vram;
extern int xsize;
extern int ysize;

int cursor_x, cursor_y;
bool use_vram;
SerialPort* serial_port;

void ResetCursorPosition() {
  cursor_x = 0;
  cursor_y = 0;
}

void EnableVideoModeForConsole() {
  if (!vram)
    return;
  use_vram = true;
}

void SetSerialForConsole(SerialPort* p) {
  serial_port = p;
}

void PutChar(char c) {
  if (serial_port)
    serial_port->SendChar(c);
  if (!use_vram) {
    if (c == '\n') {
      EFI::ConOut::PutChar('\r');
      EFI::ConOut::PutChar('\n');
      return;
    }
    EFI::ConOut::PutChar(c);
    return;
  }
  if (c == '\n') {
    cursor_y += 16;
    cursor_x = 0;
  } else if (c == '\b') {
    cursor_x -= 8;
  } else {
    DrawCharacter(c, cursor_x, cursor_y);
    cursor_x += 8;
  }
  if (cursor_x >= xsize) {
    cursor_y += 16;
    cursor_x = 0;
  } else if (cursor_x < 0) {
    cursor_y -= 16;
    cursor_x = (xsize - 8) & ~7;
  }
  if (c == '\b') {
    DrawRect(cursor_x, cursor_y, 8, 16, 0x000000);
  }
  if (cursor_y + 16 > ysize) {
    BlockTransfer(0, 0, 0, 16, xsize, ysize - 16);
    DrawRect(0, cursor_y - 16, xsize, 16, 0x000000);
    cursor_y -= 16;
  }
}

void PutString(const char* s) {
  while (*s) {
    PutChar(*(s++));
  }
}

void PutChars(const char* s, int n) {
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
    if (i == 7)
      PutString("_");
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
