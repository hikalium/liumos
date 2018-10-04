#include "liumos.h"

extern uint8_t* vram;
extern int xsize;
extern int ysize;

int cursor_x, cursor_y;

void ResetCursorPosition() {
  cursor_x = 0;
  cursor_y = 0;
}

void PutChar(char c) {
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
    for (int y = 0; y < cursor_y - 16; y++) {
      for (int x = 0; x < xsize; x++) {
        for (int i = 0; i < 4; i++) {
          vram[4 * (y * xsize + x) + i] = vram[4 * ((y + 16) * xsize + x) + i];
        }
      }
    }
    DrawRect(0, cursor_y - 16, xsize, 16, 0x000000);
    cursor_y -= 16;
  }
}

void PutString(const char* s) {
  while (*s) {
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

void PutStringAndHex(const char* s, uint64_t value) {
  PutString(s);
  PutString(": 0x");
  PutHex64(value);
  PutString("\n");
}
