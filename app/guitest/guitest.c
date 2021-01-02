#include <stdint.h>
#include <stdio.h>
#include <string.h>

// c.f. https://en.wikipedia.org/wiki/BMP_file_format

struct __attribute__((packed)) BMPFileHeader {
  uint8_t signature[2];  // "BM"
  uint32_t file_size;
  uint32_t reserved;
  uint32_t offset_to_data;  // offset in file to pixels
};

struct __attribute__((packed)) BMPInfoV3Header {
  uint32_t info_size;  // = 0x38 for Size of data follows this header
  int32_t xsize;
  int32_t ysize;
  uint16_t planes;             // = 1
  uint16_t bpp;                // = 32
  uint32_t compression_type;   // = 3: bit field, {B,G,R,A}
  uint32_t image_data_size;    // Size of data which follows this header
  uint32_t pixel_per_meter_x;  // = 0x2E23 = 300 ppi
  uint32_t pixel_per_meter_y;
  uint64_t padding0;
  uint32_t r_mask;
  uint32_t g_mask;
  uint32_t b_mask;
};

int main(int argc, char* argv[]) {
  const int w = 16;
  const int h = 16;
  uint32_t header_size_with_padding =
      (sizeof(struct BMPFileHeader) + sizeof(struct BMPInfoV3Header) + 0xF) &
      ~0xF; /* header size aligned to 16-byte boundary */

  struct BMPFileHeader file_header;
  bzero(&file_header, sizeof(file_header));
  file_header.signature[0] = 'B';
  file_header.signature[1] = 'M';
  file_header.file_size = header_size_with_padding + w * h * 4;
  file_header.offset_to_data = header_size_with_padding;

  struct BMPInfoV3Header info_header;
  bzero(&info_header, sizeof(info_header));
  info_header.info_size = sizeof(info_header);
  info_header.xsize = w;
  info_header.ysize = -h;
  info_header.planes = 1;
  info_header.bpp = 32;
  info_header.compression_type = 3;
  info_header.image_data_size =
      file_header.file_size - sizeof(file_header) - sizeof(info_header);
  info_header.pixel_per_meter_y = 0x2E23;  // 300ppi
  info_header.pixel_per_meter_x = 0x2E23;  // 300ppi
  info_header.r_mask = 0xFF0000;
  info_header.g_mask = 0x00FF00;
  info_header.b_mask = 0x0000FF;

  FILE* fp = fopen("window_16x16.bmp", "wb");
  fwrite(&file_header, 1, sizeof(file_header), fp);
  fwrite(&info_header, 1, sizeof(info_header), fp);
  for (int i = 0;
       i < header_size_with_padding - sizeof(file_header) - sizeof(info_header);
       i++) {
    fputc(0, fp);
  }
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      // BGRA
      fputc(0, fp);
      fputc(y * 16, fp);  // G
      fputc(x * 16, fp);  // R
      fputc(0, fp);
    }
  }

  return 0;
}
