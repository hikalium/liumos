/* from liumlib */
void Print(const char* s);
void Println(const char* s);

void NotImplemented(const char *s) {
  Print("Not Implemented: ");
  Println(s);
  exit(1);
}

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library;
void InitFreeType() {
  int err = FT_Init_FreeType(&library);
  if(err) {
    Print("Failed to init FreeType");
    exit(1);
  }
}

char* getenv() { return NULL; }

void BZ2_bzDecompress() {NotImplemented(__FUNCTION__);}
void BZ2_bzDecompressEnd() {NotImplemented(__FUNCTION__);}
void BZ2_bzDecompressInit() {NotImplemented(__FUNCTION__);}
void __errno_location() {NotImplemented(__FUNCTION__);}
void fcntl() {NotImplemented(__FUNCTION__);}
void free() {NotImplemented(__FUNCTION__);}
void __fxstat() {NotImplemented(__FUNCTION__);}
void hb_buffer_add_utf8(){NotImplemented(__FUNCTION__);}
void hb_buffer_clear_contents(){NotImplemented(__FUNCTION__);}
void hb_buffer_create(){NotImplemented(__FUNCTION__);}
void hb_buffer_destroy(){NotImplemented(__FUNCTION__);}
void hb_buffer_get_glyph_infos(){NotImplemented(__FUNCTION__);}
void hb_buffer_get_glyph_positions(){NotImplemented(__FUNCTION__);}
void hb_buffer_get_length(){NotImplemented(__FUNCTION__);}
void hb_buffer_guess_segment_properties(){NotImplemented(__FUNCTION__);}
void hb_font_destroy(){NotImplemented(__FUNCTION__);}
void hb_font_get_face(){NotImplemented(__FUNCTION__);}
void hb_font_set_scale(){NotImplemented(__FUNCTION__);}
void hb_ft_font_create(){NotImplemented(__FUNCTION__);}
void hb_ot_layout_collect_lookups(){NotImplemented(__FUNCTION__);}
void hb_ot_layout_lookup_collect_glyphs(){NotImplemented(__FUNCTION__);}
void hb_ot_layout_lookup_would_substitute(){NotImplemented(__FUNCTION__);}
void hb_ot_tags_from_script(){NotImplemented(__FUNCTION__);}
void hb_set_create(){NotImplemented(__FUNCTION__);}
void hb_set_destroy(){NotImplemented(__FUNCTION__);}
void hb_set_is_empty(){NotImplemented(__FUNCTION__);}
void hb_set_next(){NotImplemented(__FUNCTION__);}
void hb_set_subtract(){NotImplemented(__FUNCTION__);}
void hb_shape(){NotImplemented(__FUNCTION__);}
void inflate() {NotImplemented(__FUNCTION__);}
void inflateEnd() {NotImplemented(__FUNCTION__);}
void inflateInit2_() {NotImplemented(__FUNCTION__);}
void inflateReset() {NotImplemented(__FUNCTION__);}
void __longjmp_chk() {NotImplemented(__FUNCTION__);}
void memchr() {NotImplemented(__FUNCTION__);}
void memcmp() {NotImplemented(__FUNCTION__);}
void __memcpy_chk() {NotImplemented(__FUNCTION__);}
void memmove() {NotImplemented(__FUNCTION__);}
void munmap() {NotImplemented(__FUNCTION__);}
void png_create_info_struct() {NotImplemented(__FUNCTION__);}
void png_create_read_struct() {NotImplemented(__FUNCTION__);}
void png_destroy_read_struct() {NotImplemented(__FUNCTION__);}
void png_error(){NotImplemented(__FUNCTION__);}
void png_get_error_ptr(){NotImplemented(__FUNCTION__);}
void png_get_IHDR(){NotImplemented(__FUNCTION__);}
void png_get_io_ptr(){NotImplemented(__FUNCTION__);}
void png_get_valid(){NotImplemented(__FUNCTION__);}
void png_read_end(){NotImplemented(__FUNCTION__);}
void png_read_image(){NotImplemented(__FUNCTION__);}
void png_read_info(){NotImplemented(__FUNCTION__);}
void png_read_update_info(){NotImplemented(__FUNCTION__);}
void png_set_expand_gray_1_2_4_to_8(){NotImplemented(__FUNCTION__);}
void png_set_filler(){NotImplemented(__FUNCTION__);}
void png_set_gray_to_rgb(){NotImplemented(__FUNCTION__);}
void png_set_interlace_handling(){NotImplemented(__FUNCTION__);}
void png_set_longjmp_fn() {NotImplemented(__FUNCTION__);}
void png_set_packing(){NotImplemented(__FUNCTION__);}
void png_set_palette_to_rgb(){NotImplemented(__FUNCTION__);}
void png_set_read_fn() {NotImplemented(__FUNCTION__);}
void png_set_read_user_transform_fn(){NotImplemented(__FUNCTION__);}
void png_set_strip_16(){NotImplemented(__FUNCTION__);}
void png_set_tRNS_to_alpha(){NotImplemented(__FUNCTION__);}
void qsort() {NotImplemented(__FUNCTION__);}
void realloc() {NotImplemented(__FUNCTION__);}
void _setjmp() {NotImplemented(__FUNCTION__);}
void __sprintf_chk() {NotImplemented(__FUNCTION__);}
void __stack_chk_fail() {NotImplemented(__FUNCTION__);}
void strncpy() {NotImplemented(__FUNCTION__);}
void strrchr() {NotImplemented(__FUNCTION__);}
void strstr() {NotImplemented(__FUNCTION__);}
void strtol() {NotImplemented(__FUNCTION__);}
void fstat(){NotImplemented(__FUNCTION__);}
void longjmp(){NotImplemented(__FUNCTION__);}
void sprintf(){NotImplemented(__FUNCTION__);}
