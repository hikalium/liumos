#pragma once
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
    below_ = nullptr;
    children_ = nullptr;
    buf_ = buf;
    rect_.xsize = xsize;
    rect_.ysize = ysize;
    pixels_per_scan_line_ = pixels_per_scan_line;
    rect_.x = x;
    rect_.y = y;
  }
  void SetParent(Sheet* parent) {
    // Insert at front
    below_ = parent->children_;
    parent_ = parent;
    parent->children_ = this;
  }
  int GetXSize() { return rect_.xsize; }
  int GetYSize() { return rect_.ysize; }
  int GetPixelsPerScanLine() { return pixels_per_scan_line_; }
  int GetBufSize() {
    // Assume bytes per pixel == 4
    return rect_.ysize * pixels_per_scan_line_ * 4;
  }
  uint32_t* GetBuf() { return buf_; }
  Rect GetRect() { return rect_; }
  Rect GetClientRect() { return {0, 0, rect_.xsize, rect_.ysize}; }
  void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);
  // Flush fluhes the contents of sheet buf_ to its parent sheet.
  // This function is not recursive.
  void Flush(int px, int py, int w, int h);

 private:
  bool IsInRectY(int y) { return 0 <= y && y < rect_.ysize; }
  bool IsInRectOnParent(int x, int y) {
    return rect_.y <= y && y < rect_.y + rect_.ysize && rect_.x <= x &&
           x < rect_.x + rect_.xsize;
  }
  void TransferLineFrom(Sheet& src, int py, int px, int w);
  Sheet *parent_, *below_, *children_;
  uint32_t* buf_;
  Rect rect_;
  int pixels_per_scan_line_;
};
