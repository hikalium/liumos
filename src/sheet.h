#pragma once
#include <math.h>
#include <stdint.h>
#include "rect.h"

static inline int Absolute(int v) {
  return v > 0 ? v : -v;
}

class Sheet {
  friend class SheetPainter;

 public:
  void Init(uint32_t* buf,
            int xsize,
            int ysize,
            int pixels_per_scan_line,
            int x = 0,
            int y = 0) {
    parent_ = nullptr;
    below_ = nullptr;
    children_ = nullptr;
    buf_ = buf;
    rect_.xsize = xsize;
    rect_.ysize = ysize;
    pixels_per_scan_line_ = pixels_per_scan_line;
    rect_.x = x;
    rect_.y = y;
    map_ = nullptr;
  }
  void SetMap(Sheet** map) {
    map_ = map;
    UpdateMap(GetRect());
  }
  void SetParent(Sheet* parent) {
    // Insert at front
    below_ = parent->children_;
    parent_ = parent;
    parent_->children_ = this;
    parent_->UpdateMap(GetRect());
    Flush();
  }
  void SetPosition(int x, int y) {
    const auto prev_rect = GetRect();
    rect_.x = x;
    rect_.y = y;
    if (!parent_) {
      return;
    }
    parent_->UpdateMap(prev_rect);
    FlushInParent(prev_rect.x, prev_rect.y, GetXSize(), GetYSize());
    parent_->UpdateMap(GetRect());
    FlushInParent(GetX(), GetY(), GetXSize(), GetYSize());
  }
  void MoveRelative(int dx, int dy) { SetPosition(GetX() + dx, GetY() + dy); }
  int GetX() const { return rect_.x; }
  int GetY() const { return rect_.y; }
  int GetXSize() const { return rect_.xsize; }
  int GetYSize() const { return rect_.ysize; }
  int GetPixelsPerScanLine() const { return pixels_per_scan_line_; }
  int GetBufSize() const {
    // Assume bytes per pixel == 4
    return rect_.ysize * pixels_per_scan_line_ * 4;
  }
  uint32_t* GetBuf() const { return buf_; }
  Rect GetRect() const { return rect_; }
  Rect GetClientRect() const { return {0, 0, rect_.xsize, rect_.ysize}; }
  void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);
  // Flush fluhes the contents of sheet buf_ to its parent sheet.
  // This function is not recursive.
  void FlushInParent(int px, int py, int w, int h);
  void Flush(int px, int py, int w, int h);
  void Flush() { Flush(0, 0, rect_.xsize, rect_.ysize); };

 private:
  void UpdateMap(Rect target) {
    if (!map_) {
      return;
    }
    target = target.GetIntersectionWith(GetClientRect());
    for (int y = target.y; y < target.GetBottom(); y++) {
      for (int x = target.x; x < target.GetRight(); x++) {
        map_[y * GetXSize() + x] = nullptr;
      }
    }
    for (Sheet* s = children_; s; s = s->below_) {
      Rect in_view = target.GetIntersectionWith(s->GetRect());
      for (int y = in_view.y; y < in_view.y + in_view.ysize; y++) {
        for (int x = in_view.x; x < in_view.x + in_view.xsize; x++) {
          if (map_[y * GetXSize() + x]) {
            continue;
          }
          map_[y * GetXSize() + x] = s;
        }
      }
    }
  }
  bool IsInRectY(int y) { return 0 <= y && y < rect_.ysize; }
  bool IsInRectOnParent(int x, int y) {
    return rect_.y <= y && y < rect_.y + rect_.ysize && rect_.x <= x &&
           x < rect_.x + rect_.xsize;
  }
  void TransferLineFrom(Sheet& src, int py, int px, int w);
  Sheet *parent_, *below_, *children_;
  uint32_t* buf_;
  Sheet** map_;
  Rect rect_;
  int pixels_per_scan_line_;
};
