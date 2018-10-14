#include "liumos.h"

extern uint8_t* vram;
extern int xsize;
extern int ysize;

int cursor_x, cursor_y;
bool use_vram;

void ResetCursorPosition() {
  cursor_x = 0;
  cursor_y = 0;
}

void EnableVideoModeForConsole() {
  if (!vram)
    return;
  use_vram = true;
}

void PutChar(char c) {
  if (!use_vram) {
    if (c == '\n') {
      EFIPutChar('\r');
      EFIPutChar('\n');
      return;
    }
    EFIPutChar(c);
    return;
  }
  ClearIntFlag();
  if (c == '\n') {
    cursor_y += 16;
    cursor_x = 0;
  } else {
    DrawCharacter(c, cursor_x, cursor_y);
    cursor_x += 8;
  }
  if (cursor_x > xsize) {
    cursor_y += 16;
    cursor_x = 0;
  }
  if (cursor_y + 16 > ysize) {
    BlockTransfer(0, 0, 0, 16, xsize, ysize - 16);
    DrawRect(0, cursor_y - 16, xsize, 16, 0x000000);
    cursor_y -= 16;
  }
  StoreIntFlag();
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
