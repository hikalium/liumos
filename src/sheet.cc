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

void Sheet::TransferLineFrom(Sheet& src, int py, int px, int w) {
  // Transfer line range [px, px + w) at row py from src.
  // px, py is coordinates on this sheet.
  // Given range shold be in src sheet.
  if (w <= 0)
    return;
  if (w & 1) {
    RepeatMove4Bytes(w, &buf_[py * pixels_per_scan_line_ + px],
                     &src.buf_[(py - src.rect_.y) * src.pixels_per_scan_line_ +
                               (px - src.rect_.x)]);
  } else {
    RepeatMove8Bytes(w >> 1, &buf_[py * pixels_per_scan_line_ + px],
                     &src.buf_[(py - src.rect_.y) * src.pixels_per_scan_line_ +
                               (px - src.rect_.x)]);
  }
}

void Sheet::Flush(int rx, int ry, int rw, int rh) {
  // Transfer (ax, ay)(aw * ah) area in this sheet to parent
  if (!parent_)
    return;
  auto local_area = GetClientRect().GetIntersectionWith({rx, ry, rw, rh});
  // calc area rect in parent
  auto [tx, ty, tw, th] = parent_->GetClientRect().GetIntersectionWith(
      {local_area.x + rect_.x, local_area.y + rect_.y, local_area.xsize,
       local_area.ysize});
  if (tw <= 0 || th <= 0) {
    // No need to flush when the given area is empty
    return;
  }
  assert(0 <= tx && 0 <= ty && (tx + tw) <= parent_->rect_.xsize &&
         (ty + th) <= parent_->rect_.ysize);
  for (int y = ty; y < ty + th; y++) {
    int tbegin = tx;
    for (int x = tx; x < tx + tw; x++) {
      bool is_covered = false;
      for (Sheet* s = parent_->children_; s && s != this; s = s->below_) {
        if (s->IsInRectOnParent(x, y)) {
          is_covered = true;
          break;
        }
      }
      if (!is_covered)
        continue;
      parent_->TransferLineFrom(*this, y, tbegin, x - tbegin);
      tbegin = x + 1;
    }
    if (tx != tbegin) {
      // this line is overwrapped with other sheets and already transfered in
      // the loop above.
      // Transfer last segment of line.
      parent_->TransferLineFrom(*this, y, tbegin, (tx + tw) - tbegin);
      continue;
    }
    parent_->TransferLineFrom(*this, y, tx, tw);
  }
}

void Sheet::FlushRecursive(int rx, int ry, int rw, int rh) {
  if (!parent_)
    return;
  Flush(rx, ry, rw, rh);
  parent_->FlushRecursive(rx + rect_.x, ry + rect_.y, rw, rh);
}
