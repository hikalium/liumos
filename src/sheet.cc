#include "sheet.h"
#include "asm.h"

void Sheet::BlockTransfer(int to_x,
                          int to_y,
                          int from_x,
                          int from_y,
                          int w,
                          int h) {
  uint32_t* b32 = reinterpret_cast<uint32_t*>(buf_);
  if (w & 1) {
    for (int dy = 0; dy < h; dy++) {
      RepeatMove4Bytes(
          w, &b32[(to_y + dy) * pixels_per_scan_line_ + (to_x)],
          &b32[(from_y + dy) * pixels_per_scan_line_ + (from_x + 0)]);
    }
  } else {
    for (int dy = 0; dy < h; dy++) {
      RepeatMove8Bytes(
          w / 2, &b32[(to_y + dy) * pixels_per_scan_line_ + (to_x)],
          &b32[(from_y + dy) * pixels_per_scan_line_ + (from_x + 0)]);
    }
  }
  Flush(to_x, to_y, w, h);
}

void Sheet::Flush(int px, int py, int w, int h) {
  // Assume parent has the same resolution with this sheet
  if (!parent_)
    return;
  if (w & 1) {
    for (int y = py; y < py + h; y++) {
      RepeatMove4Bytes(
          w,
          &reinterpret_cast<uint32_t*>(
              parent_->buf_)[y * pixels_per_scan_line_ + px],
          &reinterpret_cast<uint32_t*>(buf_)[y * pixels_per_scan_line_ + px]);
    }
  } else {
    for (int y = py; y < py + h; y++) {
      RepeatMove8Bytes(
          w / 2,
          &reinterpret_cast<uint32_t*>(
              parent_->buf_)[y * pixels_per_scan_line_ + px],
          &reinterpret_cast<uint32_t*>(buf_)[y * pixels_per_scan_line_ + px]);
    }
  }
}
