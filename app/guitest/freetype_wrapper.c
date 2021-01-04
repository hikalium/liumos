#include <stdint.h>
#include "../liumlib/liumlib.h"

#include <ft2build.h>
#include FT_FREETYPE_H

extern unsigned char source_code_pro_medium_ttf[];
extern unsigned int source_code_pro_medium_ttf_len;

// c.f. http://ncl.sakura.ne.jp/doc/ja/comp/freetype-memo.html

FT_Library library;
FT_Face face;
void InitFreeType() {
  int err;
  err = FT_Init_FreeType(&library);
  if (err) {
    Print("Failed to init FreeType");
    exit(1);
  }
  err = FT_New_Memory_Face(library, source_code_pro_medium_ttf,
                           source_code_pro_medium_ttf_len, 0, &face);
  if (err) {
    Print("Failed to load font face");
    exit(1);
  }
  err = FT_Set_Pixel_Sizes(face, 64, 64);
  if (err) {
    Print("Failed to set pixel sizes");
    exit(1);
  }
  int charcode = 'A';
  err = FT_Load_Char(face, charcode, 0);
  if (err) {
    Print("Failed to load char");
    exit(1);
  }
  err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
  if (err) {
    Print("Failed to render glyph");
    exit(1);
  }
  FT_Bitmap* bm = &face->glyph->bitmap;
  int row, col, bit, c;
  for (row = 0; row < bm->rows; row++) {
    for (col = 0; col < bm->pitch; col++) {
      c = bm->buffer[bm->pitch * row + col];
      for (bit = 7; bit >= 0; bit--) {
        if (((c >> bit) & 1) == 0)
          Print("  ");
        else
          Print("##");
      }
    }
    Print("\n");
  }
}

char* getenv() {
  return NULL;
}

void BZ2_bzDecompress() {
  NotImplemented(__FUNCTION__);
}
void BZ2_bzDecompressEnd() {
  NotImplemented(__FUNCTION__);
}
void BZ2_bzDecompressInit() {
  NotImplemented(__FUNCTION__);
}
void __errno_location() {
  NotImplemented(__FUNCTION__);
}
void fcntl() {
  NotImplemented(__FUNCTION__);
}
void free() {
  NotImplemented(__FUNCTION__);
}
void __fxstat() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_add_utf8() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_clear_contents() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_create() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_destroy() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_get_glyph_infos() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_get_glyph_positions() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_get_length() {
  NotImplemented(__FUNCTION__);
}
void hb_buffer_guess_segment_properties() {
  NotImplemented(__FUNCTION__);
}
void hb_font_destroy() {
  NotImplemented(__FUNCTION__);
}
void hb_font_get_face() {
  NotImplemented(__FUNCTION__);
}
void hb_font_set_scale() {
  NotImplemented(__FUNCTION__);
}
void hb_ft_font_create() {
  NotImplemented(__FUNCTION__);
}
void hb_ot_layout_collect_lookups() {
  NotImplemented(__FUNCTION__);
}
void hb_ot_layout_lookup_collect_glyphs() {
  NotImplemented(__FUNCTION__);
}
void hb_ot_layout_lookup_would_substitute() {
  NotImplemented(__FUNCTION__);
}
void hb_ot_tags_from_script() {
  NotImplemented(__FUNCTION__);
}
void hb_set_create() {
  NotImplemented(__FUNCTION__);
}
void hb_set_destroy() {
  NotImplemented(__FUNCTION__);
}
void hb_set_is_empty() {
  NotImplemented(__FUNCTION__);
}
void hb_set_next() {
  NotImplemented(__FUNCTION__);
}
void hb_set_subtract() {
  NotImplemented(__FUNCTION__);
}
void hb_shape() {
  NotImplemented(__FUNCTION__);
}
void inflate() {
  NotImplemented(__FUNCTION__);
}
void inflateEnd() {
  NotImplemented(__FUNCTION__);
}
void inflateInit2_() {
  NotImplemented(__FUNCTION__);
}
void inflateReset() {
  NotImplemented(__FUNCTION__);
}
void __longjmp_chk() {
  NotImplemented(__FUNCTION__);
}
void __memcpy_chk() {
  NotImplemented(__FUNCTION__);
}
void munmap() {
  NotImplemented(__FUNCTION__);
}
void png_create_info_struct() {
  NotImplemented(__FUNCTION__);
}
void png_create_read_struct() {
  NotImplemented(__FUNCTION__);
}
void png_destroy_read_struct() {
  NotImplemented(__FUNCTION__);
}
void png_error() {
  NotImplemented(__FUNCTION__);
}
void png_get_error_ptr() {
  NotImplemented(__FUNCTION__);
}
void png_get_IHDR() {
  NotImplemented(__FUNCTION__);
}
void png_get_io_ptr() {
  NotImplemented(__FUNCTION__);
}
void png_get_valid() {
  NotImplemented(__FUNCTION__);
}
void png_read_end() {
  NotImplemented(__FUNCTION__);
}
void png_read_image() {
  NotImplemented(__FUNCTION__);
}
void png_read_info() {
  NotImplemented(__FUNCTION__);
}
void png_read_update_info() {
  NotImplemented(__FUNCTION__);
}
void png_set_expand_gray_1_2_4_to_8() {
  NotImplemented(__FUNCTION__);
}
void png_set_filler() {
  NotImplemented(__FUNCTION__);
}
void png_set_gray_to_rgb() {
  NotImplemented(__FUNCTION__);
}
void png_set_interlace_handling() {
  NotImplemented(__FUNCTION__);
}
void png_set_longjmp_fn() {
  NotImplemented(__FUNCTION__);
}
void png_set_packing() {
  NotImplemented(__FUNCTION__);
}
void png_set_palette_to_rgb() {
  NotImplemented(__FUNCTION__);
}
void png_set_read_fn() {
  NotImplemented(__FUNCTION__);
}
void png_set_read_user_transform_fn() {
  NotImplemented(__FUNCTION__);
}
void png_set_strip_16() {
  NotImplemented(__FUNCTION__);
}
void png_set_tRNS_to_alpha() {
  NotImplemented(__FUNCTION__);
}
void qsort() {
  NotImplemented(__FUNCTION__);
}
void __sprintf_chk() {
  NotImplemented(__FUNCTION__);
}
void __stack_chk_fail() {
  NotImplemented(__FUNCTION__);
}
void fstat() {
  NotImplemented(__FUNCTION__);
}
void fread() {
  NotImplemented(__FUNCTION__);
}
void fopen() {
  NotImplemented(__FUNCTION__);
}
void fsync() {
  NotImplemented(__FUNCTION__);
}
void fseek() {
  NotImplemented(__FUNCTION__);
}
void ftell() {
  NotImplemented(__FUNCTION__);
}
void fclose() {
  NotImplemented(__FUNCTION__);
}

void* memchr(const void*a, int b, unsigned long c) {
  NotImplemented(__FUNCTION__);
}
int memcmp(const void* a, const void* b, unsigned long c) {
  NotImplemented(__FUNCTION__);
}
char* strncpy(char* a, const char* b, unsigned long c) {
  NotImplemented(__FUNCTION__);
}
char* strrchr(const char* a, int b) {
  NotImplemented(__FUNCTION__);
}
long strtol(const char* a, char** b, int c) {
  NotImplemented(__FUNCTION__);
}
int sprintf(char* a, const char* b, ...) {
  NotImplemented(__FUNCTION__);
}

int setjmp(long long *buf) {
  //NotImplemented(__FUNCTION__);
  return __builtin_setjmp((void**)buf);
}
void longjmp() {
  NotImplemented(__FUNCTION__);
}
