#include "liumos.h"

extern uint8_t* vram;
extern int xsize;
extern int ysize;

void DrawCharacter(char c, int px, int py) {
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      uint8_t col = ((font[(uint8_t)c][dy] >> (7 - dx)) & 1) * 0xff;
      int x = px + dx;
      int y = py + dy;
      vram[4 * (y * xsize + x) + 0] = col;
      vram[4 * (y * xsize + x) + 1] = col;
      vram[4 * (y * xsize + x) + 2] = col;
      // vram[4 * (y * xsize + x) + 3] = col;
    }
  }
}

void DrawRect(int px, int py, int w, int h, uint32_t col) {
  for (int y = py; y < py + h; y++) {
    for (int x = px; x < px + w; x++) {
      for (int i = 0; i < 4; i++) {
        vram[4 * (y * xsize + x) + i] = (col >> (i * 8)) & 0xff;
      }
    }
  }
}
