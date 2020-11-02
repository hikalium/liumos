#include <math.h>

#include "kernel.h"
#include "liumos.h"
#include "sheet.h"

class PolygonCube {
 public:
  PolygonCube() {
    sheet_ = new Sheet();
    sheet_->Init(buf_, width, height, width,
                 liumos->screen_sheet->GetXSize() - width - 64, 64);
    sheet_->SetParent(liumos->vram_sheet);
  }
  void Draw(void) {
    // http://k.osask.jp/wiki/?p20191125a
    constexpr double kToRad = 3.14159265358979323 / 0x8000;
    thx_ = (thx_ + 182) & 0xffff;
    thy_ = (thy_ + 273) & 0xffff;
    thz_ = (thz_ + 364) & 0xffff;
    double xp = cos(thx_ * kToRad), xa = sin(thx_ * kToRad);
    double yp = cos(thy_ * kToRad), ya = sin(thy_ * kToRad);
    double zp = cos(thz_ * kToRad), za = sin(thz_ * kToRad);
    for (int i = 0; i < 8; i++) {
      double xt, yt, zt;
      zt = vertz[i] * xp + verty[i] * xa;
      yt = verty[i] * xp - vertz[i] * xa;
      xt = vertx[i] * yp + zt * ya;
      vz_[i] = zt * yp - vertx[i] * ya;
      vx_[i] = xt * zp - yt * za;
      vy_[i] = yt * zp + xt * za;
    }
    for (int i = 0; i < 6; i++) {
      const int l = i * 4;
      centerz4_[i] = vz_[squar[l + 0]] + vz_[squar[l + 1]] + vz_[squar[l + 2]] +
                     vz_[squar[l + 3]] + 1024.0;
    }
    FillRect(40, 0, 160, 160, 0x000000);
    DrawObj();
    sheet_->Flush(0, 0, width, height);
  }

 private:
  void FillRect(int x, int y, int w, int h, uint32_t c) {
    SheetPainter::DrawRect(*sheet_, x, y, w, h, c);
  }

  void DrawObj() {
    for (int i = 0; i < 8; i++) {
      double t = 300.0 / (vz_[i] + 400.0);
      scx_[i] = (vx_[i] * t) + 128;
      scy_[i] = (vy_[i] * t) + 80;
    }
    for (;;) {
      double max = 0.0;
      int j = -1, k;
      for (k = 0; k < 6; k++) {
        if (max < centerz4_[k]) {
          max = centerz4_[k];
          j = k;
        }
      }
      if (j < 0)
        break;
      int i = j * 4;
      centerz4_[j] = 0.0;
      double e0x = vx_[squar[i + 1]] - vx_[squar[i + 0]];
      double e0y = vy_[squar[i + 1]] - vy_[squar[i + 0]];
      double e1x = vx_[squar[i + 2]] - vx_[squar[i + 1]];
      double e1y = vy_[squar[i + 2]] - vy_[squar[i + 1]];
      if (e0x * e1y <= e0y * e1x)
        DrawPoly(j);
    }
  }

  void DrawPoly(int j) {
    int i = j * 4, i1 = i + 3;
    int p0x = scx_[squar[i1]], p0y = scy_[squar[i1]], p1x, p1y;
    int y, ymin = 0x7fffffff, ymax = 0, x, dx, y0, y1;
    int *buf, buf0[160], buf1[160];
    int c = col[j];
    for (; i <= i1; i++) {
      p1x = scx_[squar[i]];
      p1y = scy_[squar[i]];
      if (ymin > p1y)
        ymin = p1y;
      if (ymax < p1y)
        ymax = p1y;
      if (p0y != p1y) {
        if (p0y < p1y) {
          buf = buf0;
          y0 = p0y;
          y1 = p1y;
          dx = p1x - p0x;
          x = p0x;
        } else {
          buf = buf1;
          y0 = p1y;
          y1 = p0y;
          dx = p0x - p1x;
          x = p1x;
        }
        x <<= 16;
        dx = (dx << 16) / (y1 - y0);
        if (dx >= 0)
          x += 0x8000;
        else
          x -= 0x8000;
        for (y = y0; y <= y1; y++) {
          buf[y] = x >> 16;
          x += dx;
        }
      }
      p0x = p1x;
      p0y = p1y;
    }
    for (y = ymin; y <= ymax; y++) {
      p0x = buf0[y];
      p1x = buf1[y];
      if (p0x <= p1x)
        FillRect(p0x, y, p1x - p0x + 1, 1, c);
      else
        FillRect(p1x, y, p0x - p1x + 1, 1, c);
    }
  }
  Sheet* sheet_;
  static constexpr int width = 256;
  static constexpr int height = 160;
  static constexpr int squar[24] = {0, 4, 6, 2, 1, 3, 7, 5, 0, 2, 3, 1,
                                    0, 1, 5, 4, 4, 5, 7, 6, 6, 7, 3, 2};
  static constexpr uint32_t col[6] = {0xff0000, 0x00ff00, 0x0000ff,
                                      0xffff00, 0xff00ff, 0x00ffff};

  static constexpr double vertx[8] = {50.0,  50.0,  50.0,  50.0,
                                      -50.0, -50.0, -50.0, -50.0};
  static constexpr double verty[8] = {50.0, 50.0, -50.0, -50.0,
                                      50.0, 50.0, -50.0, -50.0};
  static constexpr double vertz[8] = {50.0, -50.0, 50.0, -50.0,
                                      50.0, -50.0, 50.0, -50.0};
  uint32_t buf_[width * height];
  double vx_[8], vy_[8], vz_[8];
  double centerz4_[6];
  int scx_[8], scy_[8];
  int thx_, thy_, thz_;
};

void CellularAutomaton() {
  constexpr int map_ysize_shift = 4;
  constexpr int map_xsize_shift = 5;
  constexpr int pixel_size = 8;

  constexpr int ysize_mask = (1 << map_ysize_shift) - 1;
  constexpr int xsize_mask = (1 << map_xsize_shift) - 1;
  constexpr int ysize = 1 << map_ysize_shift;
  constexpr int xsize = 1 << map_xsize_shift;
  static char map[ysize * xsize];
  constexpr int canvas_ysize = ysize * pixel_size;
  constexpr int canvas_xsize = xsize * pixel_size;

  map[(ysize / 2 - 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 - 1) * xsize + (xsize / 2 + 2)] = 1;

  map[(ysize / 2) * xsize + (xsize / 2 - 4)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 2)] = 1;
  map[(ysize / 2) * xsize + (xsize / 2 + 3)] = 1;

  map[(ysize / 2 + 1) * xsize + (xsize / 2 - 3)] = 1;
  map[(ysize / 2 + 1) * xsize + (xsize / 2 + 2)] = 1;

  while (1) {
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int count = 0;
        for (int p = -1; p <= 1; p++)
          for (int q = -1; q <= 1; q++)
            count +=
                map[((y + p) & ysize_mask) * xsize + ((x + q) & xsize_mask)] &
                1;
        count -= map[y * xsize + x] & 1;
        if ((map[y * xsize + x] && (count == 2 || count == 3)) ||
            (!map[y * xsize + x] && count == 3))
          map[y * xsize + x] |= 2;
      }
    }
    for (int y = 0; y < ysize; y++) {
      for (int x = 0; x < xsize; x++) {
        int p = map[y * xsize + x];
        int col = 0x000000;
        if (p & 1)
          col = 0xff0088 * (p & 2) + 0x00cc00 * (p & 1);
        map[y * xsize + x] >>= 1;
        SheetPainter::DrawRect(
            *liumos->screen_sheet,
            liumos->screen_sheet->GetXSize() - canvas_xsize + x * pixel_size,
            y * pixel_size, pixel_size, pixel_size, col);
      }
    }
    liumos->screen_sheet->Flush(liumos->screen_sheet->GetXSize() - canvas_xsize,
                                0, canvas_xsize, canvas_ysize);
    HPET::GetInstance().BusyWait(200);
  }
}

void SubTask() {
  PolygonCube pcube;
  kprintf("pcube size: 0x%X\n", sizeof(pcube));
  for (;;) {
    pcube.Draw();
    HPET::GetInstance().BusyWait(10);
  }
}
