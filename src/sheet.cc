#include "sheet.h"

#include "asm.h"
#include "generic.h"

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

void Sheet::FlushInParent(int rx, int ry, int rw, int rh) {
  auto [tx, ty, tw, th] =
      parent_->GetClientRect().GetIntersectionWith({rx, ry, rw, rh});
  if (tw <= 0 || th <= 0) {
    // No need to flush when the given area is empty
    return;
  }
  if (!parent_ || !parent_->map_) {
    return;
  }
  assert(0 <= tx && 0 <= ty && (tx + tw) <= parent_->rect_.xsize &&
         (ty + th) <= parent_->rect_.ysize);

  const auto parent_map = parent_->map_;
  const auto parent_line_size = parent_->GetPixelsPerScanLine();
  const auto parent_buf = parent_->buf_;
  for (int y = ty; y < ty + th; y++) {
    for (int x = tx; x < tx + tw; x++) {
      if (parent_map[y * parent_line_size + x] != this) {
        continue;
      }
      parent_buf[y * parent_line_size + x] =
          buf_[(y - rect_.y) * pixels_per_scan_line_ + (x - rect_.x)];
    }
  }
}

void Sheet::Flush(int rx, int ry, int rw, int rh) {
  // calc area rect in parent
  auto [tx, ty, tw, th] = parent_->GetClientRect().GetIntersectionWith(
      GetRect().GetIntersectionWith({rx + GetX(), ry + GetY(), rw, rh}));
  FlushInParent(tx, ty, tw, th);
}
