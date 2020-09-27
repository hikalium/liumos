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

void Sheet::Flush(int tx, int ty, int tw, int th) {
  // Transfer (ax, ay)(aw * ah) area to parent
  if (!parent_)
    return;
  auto [ax, ay, aw, ah] = GetClientRect().GetIntersectionWith({tx, ty, tw, th});
  if (aw <= 0 || ah <= 0) {
    // the area cannot be negative
    return;
  }
  assert(0 <= ax && 0 <= ay && (ax + aw) <= rect_.xsize &&
         (ay + ah) <= rect_.ysize);
  const int px = ax + rect_.x;
  const int py = ay + rect_.y;
  if (px >= parent_->rect_.xsize || py >= parent_->rect_.ysize ||
      (px + aw) < 0 || (py + ah) < 0) {
    // This sheet has no intersections with its parent.
    return;
  }
  for (int y = py; y < py + ah; y++) {
    if (!parent_->IsInRectY(y))
      continue;
    const int begin = (0 < px) ? px : 0;
    const int end =
        (px + aw < parent_->rect_.xsize) ? (px + aw) : parent_->rect_.xsize;
    const int tw = end - begin;
    assert(tw > 0);
    int tbegin = begin;
    for (int x = begin; x < end; x++) {
      bool is_overlapped = false;
      for (Sheet* s = front_; s && !is_overlapped; s = s->front_) {
        is_overlapped = s->IsInRectOnParent(x, y);
      }
      if (is_overlapped) {
        int tend = x;
        if (tend - tbegin) {
          RepeatMove4Bytes(
              tend - tbegin,
              &parent_->buf_[y * parent_->pixels_per_scan_line_ + tbegin],
              &buf_[(y - rect_.y) * pixels_per_scan_line_ +
                    (tbegin - rect_.x)]);
        }
        tbegin = x + 1;
        continue;
      }
    }
    int tend = end;
    if (tend - tbegin) {
      if ((tend - tbegin) & 1) {
        RepeatMove4Bytes(
            tend - tbegin,
            &parent_->buf_[y * parent_->pixels_per_scan_line_ + tbegin],
            &buf_[(y - rect_.y) * pixels_per_scan_line_ + (tbegin - rect_.x)]);
      } else {
        RepeatMove8Bytes(
            (tend - tbegin) >> 1,
            &parent_->buf_[y * parent_->pixels_per_scan_line_ + tbegin],
            &buf_[(y - rect_.y) * pixels_per_scan_line_ + (tbegin - rect_.x)]);
      }
    }
  }
}
