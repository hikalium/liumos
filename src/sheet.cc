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

void Sheet::Flush(int fx, int fy, int w, int h) {
  // Transfer (px, py)(w * h) area to parent
  if (!parent_)
    return;
  assert(0 <= fx && 0 <= fy && (fx + w) <= xsize_ && (fy + h) <= ysize_);
  const int px = fx + x_;
  const int py = fy + y_;
  if (px >= parent_->xsize_ || py >= parent_->ysize_ || (px + w) < 0 ||
      (py + h) < 0) {
    // This sheet has no intersections with its parent.
    return;
  }
  for (int y = py; y < py + h; y++) {
    if (!parent_->IsInRectY(y))
      continue;
    const int begin = (0 < px) ? px : 0;
    const int end = (px + w < parent_->xsize_) ? (px + w) : parent_->xsize_;
    const int tw = end - begin;
    assert(tw > 0);
    for (int x = begin; x < end; x++) {
      bool is_overlapped = false;
      for (Sheet* s = front_; s && !is_overlapped; s = s->front_) {
        is_overlapped = s->IsInRectOnParent(x, y);
      }
      if (is_overlapped)
        continue;
      parent_->buf_[y * parent_->pixels_per_scan_line_ + x] =
          buf_[(y - y_) * pixels_per_scan_line_ + (x - x_)];
    }
  }
}
