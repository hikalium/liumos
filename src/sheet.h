#pragma once
#include <stdint.h>

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
    buf_ = buf;
    xsize_ = xsize;
    ysize_ = ysize;
    pixels_per_scan_line_ = pixels_per_scan_line;
    x_ = x;
    y_ = y;
  }
  void SetParent(Sheet* parent) { parent_ = parent; }
  int GetXSize() { return xsize_; }
  int GetYSize() { return ysize_; }
  int GetPixelsPerScanLine() { return pixels_per_scan_line_; }
  int GetBufSize() {
    // Assume bytes per pixel == 4
    return ysize_ * pixels_per_scan_line_ * 4;
  }
  uint32_t* GetBuf() { return buf_; }
  void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);
  void Flush(int px, int py, int w, int h);

 private:
  bool IsInRectY(int y) { return 0 <= y && y < ysize_; }
  Sheet* parent_;
  uint32_t* buf_;
  int xsize_, ysize_;
  int x_, y_;
  int pixels_per_scan_line_;
};
