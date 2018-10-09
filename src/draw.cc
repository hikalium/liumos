#include "liumos.h"

extern uint8_t* vram;
extern int xsize;
extern int ysize;
extern int pixels_per_scan_line;

void DrawCharacter(char c, int px, int py) {
  if (!vram)
    return;
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      uint8_t col = ((font[(uint8_t)c][dy] >> (7 - dx)) & 1) * 0xff;
      int x = px + dx;
      int y = py + dy;
      vram[4 * (y * pixels_per_scan_line + x) + 0] = col;
      vram[4 * (y * pixels_per_scan_line + x) + 1] = col;
      vram[4 * (y * pixels_per_scan_line + x) + 2] = col;
    }
  }
}

void DrawRect(int px, int py, int w, int h, uint32_t col) {
  if (!vram)
    return;
  for (int y = py; y < py + h; y++) {
    for (int x = px; x < px + w; x++) {
      for (int i = 0; i < 4; i++) {
        vram[4 * (y * pixels_per_scan_line + x) + i] = (col >> (i * 8)) & 0xff;
      }
    }
  }
}

void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h) {
  for (int dy = 0; dy < h; dy++) {
    for (int dx = 0; dx < w; dx++) {
      for (int i = 0; i < 4; i++) {
        vram[4 * ((to_y + dy) * pixels_per_scan_line + (to_x + dx)) + i] =
            vram[4 * ((from_y + dy) * pixels_per_scan_line + (from_x + dx)) +
                 i];
      }
    }
  }
}
