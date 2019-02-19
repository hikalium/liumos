#include "liumos.h"

void Sheet::DrawCharacter(char c, int px, int py) {
  if (!buf_)
    return;
  for (int dy = 0; dy < 16; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      uint8_t col = ((font[(uint8_t)c][dy] >> (7 - dx)) & 1) * 0xff;
      int x = px + dx;
      int y = py + dy;
      buf_[4 * (y * pixels_per_scan_line_ + x) + 0] = col;
      buf_[4 * (y * pixels_per_scan_line_ + x) + 1] = col;
      buf_[4 * (y * pixels_per_scan_line_ + x) + 2] = col;
    }
  }
  Flush(px, py, 8, 16);
}

void Sheet::DrawRect(int px, int py, int w, int h, uint32_t col) {
  if (!buf_)
    return;
  for (int y = py; y < py + h; y++) {
    for (int x = px; x < px + w; x++) {
      for (int i = 0; i < 4; i++) {
        buf_[4 * (y * pixels_per_scan_line_ + x) + i] = (col >> (i * 8)) & 0xff;
      }
    }
  }
  Flush(px, py, w, h);
}

void Sheet::BlockTransfer(int to_x,
                          int to_y,
                          int from_x,
                          int from_y,
                          int w,
                          int h) {
  for (int dy = 0; dy < h; dy++) {
    for (int dx = 0; dx < w; dx++) {
      for (int i = 0; i < 4; i++) {
        buf_[4 * ((to_y + dy) * pixels_per_scan_line_ + (to_x + dx)) + i] =
            buf_[4 * ((from_y + dy) * pixels_per_scan_line_ + (from_x + dx)) +
                 i];
      }
    }
  }
  Flush(to_x, to_y, w, h);
}

void Sheet::Flush(int px, int py, int w, int h) {
  // Assume parent has the same resolution with this sheet
  if (!parent_)
    return;
  for (int y = py; y < py + h; y++) {
    for (int x = px; x < px + w; x++) {
      reinterpret_cast<uint32_t*>(
          parent_->buf_)[y * pixels_per_scan_line_ + x] =
          reinterpret_cast<uint32_t*>(buf_)[y * pixels_per_scan_line_ + x];
    }
  }
}
