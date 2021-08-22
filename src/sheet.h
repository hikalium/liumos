#pragma once
#include <math.h>
#include <stdint.h>
#include "rect.h"

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
    upper_ = nullptr;
    bottom_child_ = nullptr;
    buf_ = buf;
    rect_.xsize = xsize;
    rect_.ysize = ysize;
    pixels_per_scan_line_ = pixels_per_scan_line;
    rect_.x = x;
    rect_.y = y;
    map_ = nullptr;
    is_topmost_ = false;
    is_alpha_enabled_ = false;
  }
  void SetMap(Sheet** map) {
    map_ = map;
    UpdateMap(GetRect());
  }
  void SetParent(Sheet* parent) {
    parent_ = parent;
    if (!parent_) {
      return;
    }
    // Insert at front
    Sheet** topmost_upper_holder_ = &parent_->bottom_child_;
    while (*topmost_upper_holder_ && !(*topmost_upper_holder_)->is_topmost_) {
      topmost_upper_holder_ = &(*topmost_upper_holder_)->upper_;
    }
    upper_ = *topmost_upper_holder_;
    *topmost_upper_holder_ = this;
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
    for (Sheet* s = parent_->bottom_child_; s && s != this; s = s->upper_) {
      s->FlushInParent(prev_rect.x, prev_rect.y, GetXSize(), GetYSize());
    }
    parent_->UpdateMap(GetRect());
    FlushInParent(GetX(), GetY(), GetXSize(), GetYSize());
  }
  void SetTopmost(bool is_topmost) { is_topmost_ = is_topmost; }
  bool IsTopmost() { return is_topmost_; }
  void SetLocked(bool is_locked) { is_locked_ = is_locked; }
  bool IsLocked() { return is_locked_; }
  void SetAlphaEnabled(bool is_enabled) {
    is_alpha_enabled_ = is_enabled;
    if (is_alpha_enabled_) {
      // Update map and flush to reflect the alpha change
      MoveRelative(0, 0);
    }
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
  Sheet* GetChildAtBottom() { return bottom_child_; }
  Sheet* GetUpper() { return upper_; }

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
    for (Sheet* s = bottom_child_; s; s = s->upper_) {
      if (s->is_alpha_enabled_) {
        // Alpha enabled
        Rect in_view = target.GetIntersectionWith(s->GetRect());
        for (int y = in_view.y; y < in_view.y + in_view.ysize; y++) {
          for (int x = in_view.x; x < in_view.x + in_view.xsize; x++) {
            if ((s->buf_[(y - s->GetY()) * s->GetPixelsPerScanLine() +
                         (x - s->GetX())] >>
                 24) == 0x00) {
              continue;
            }
            map_[y * GetPixelsPerScanLine() + x] = s;
          }
        }
      } else {
        // Alpha disabled
        Rect in_view = target.GetIntersectionWith(s->GetRect());
        for (int y = in_view.y; y < in_view.y + in_view.ysize; y++) {
          for (int x = in_view.x; x < in_view.x + in_view.xsize; x++) {
            map_[y * GetPixelsPerScanLine() + x] = s;
          }
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
  Sheet *parent_, *upper_, *bottom_child_;
  uint32_t* buf_;
  Sheet** map_;
  Rect rect_;
  int pixels_per_scan_line_;
  bool is_topmost_;
  bool is_alpha_enabled_;
  bool is_locked_;
};
