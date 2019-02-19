#include "liumos.h"

Sheet vram_sheet;

void InitGraphics() {
  vram_sheet.Init(
      static_cast<uint8_t*>(
          EFI::graphics_output_protocol->mode->frame_buffer_base),
      EFI::graphics_output_protocol->mode->info->horizontal_resolution,
      EFI::graphics_output_protocol->mode->info->vertical_resolution,
      EFI::graphics_output_protocol->mode->info->pixels_per_scan_line);
}
