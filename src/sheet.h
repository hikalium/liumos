#pragma once
#include "generic.h"

class Sheet {
 public:
  void Init(uint8_t* buf, int xsize, int ysize, int pixels_per_scan_line) {
    parent_ = nullptr;
    buf_ = buf;
    xsize_ = xsize;
    ysize_ = ysize;
    pixels_per_scan_line_ = pixels_per_scan_line;
  }
  void SetParent(Sheet* parent) { parent_ = parent; }
  int GetXSize() { return xsize_; }
  int GetYSize() { return ysize_; }
  int GetPixelsPerScanLine() { return pixels_per_scan_line_; }
  int GetBufSize() {
    // Assume bytes per pixel == 4
    return ysize_ * pixels_per_scan_line_ * 4;
  }
  uint8_t* GetBuf() { return buf_; }
  void DrawCharacter(char c, int px, int py);
  void DrawRect(int px, int py, int w, int h, uint32_t);
  void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);
  void Flush(int px, int py, int w, int h);

 private:
  Sheet* parent_;
  uint8_t* buf_;
  int xsize_, ysize_;
  int pixels_per_scan_line_;
};
