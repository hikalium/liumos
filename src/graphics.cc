#include "corefunc.h"
#include "liumos.h"

Sheet* screen_sheet;

Sheet vram_sheet_;
Sheet screen_sheet_;

void InitGraphics() {
  const EFI::GraphicsOutputProtocol::ModeInfo& mode =
      CoreFunc::GetEFI().GetGraphicsModeInfo();
  vram_sheet_.Init(static_cast<uint32_t*>(

                       mode.frame_buffer_base),
                   mode.info->horizontal_resolution,
                   mode.info->vertical_resolution,
                   mode.info->pixels_per_scan_line);
  liumos->vram_sheet = &vram_sheet_;
  screen_sheet = &vram_sheet_;
  liumos->screen_sheet = screen_sheet;
}

void InitDoubleBuffer() {
  screen_sheet_.Init(
      GetSystemDRAMAllocator().AllocPages<uint32_t*>(
          (vram_sheet_.GetBufSize() + kPageSize - 1) >> kPageSizeExponent),
      vram_sheet_.GetXSize(), vram_sheet_.GetYSize(),
      vram_sheet_.GetPixelsPerScanLine());
  screen_sheet_.SetParent(&vram_sheet_);
  screen_sheet->Flush(0, 0, screen_sheet->GetXSize(), screen_sheet->GetYSize());
  screen_sheet = &screen_sheet_;
  liumos->screen_sheet = screen_sheet;
}

void DrawPPMFile(EFIFile& file, int px, int py) {
  const uint8_t* buf = file.GetBuf();
  uint64_t buf_size = file.GetFileSize();
  if (buf[0] != 'P' || buf[1] != '3') {
    PutString("Not supported logo type (PPM 'P3' is supported)\n");
    return;
  }
  bool in_line_comment = false;
  bool is_num_read = false;
  int width = 0;
  int height = 0;
  int max_pixel_value = 0;
  uint32_t rgb = 0;
  uint32_t tmp = 0;
  int channel_count = 0;
  int x = 0, y = 0;
  for (int i = 3; i < (int)buf_size; i++) {
    uint8_t c = buf[i];
    if (in_line_comment) {
      if (c != '\n')
        continue;
      in_line_comment = false;
      continue;
    }
    if (c == '#') {
      in_line_comment = true;
      continue;
    }
    if ('0' <= c && c <= '9') {
      is_num_read = true;
      tmp *= 10;
      tmp += (uint32_t)c - '0';
      continue;
    }
    if (!is_num_read)
      continue;
    is_num_read = false;
    if (!width) {
      width = tmp;
    } else if (!height) {
      height = tmp;
    } else if (!max_pixel_value) {
      max_pixel_value = tmp;
      assert(max_pixel_value == 255);
    } else {
      rgb <<= 8;
      rgb |= tmp;
      channel_count++;
      if (channel_count == 3) {
        channel_count = 0;
        SheetPainter::DrawPoint(*liumos->screen_sheet, px + x++, py + y, rgb);
        if (x >= width) {
          x = 0;
          y++;
        }
      }
    }
    tmp = 0;
  }
  liumos->screen_sheet->Flush(px, py, width, height);
}
