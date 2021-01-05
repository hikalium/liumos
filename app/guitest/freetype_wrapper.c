#include <stdint.h>
#include "../liumlib/liumlib.h"

#include <ft2build.h>
#include FT_FREETYPE_H

extern unsigned char source_code_pro_medium_ttf[];
extern unsigned int source_code_pro_medium_ttf_len;

extern unsigned char koruri_regular_subset_ttf[];
extern unsigned int koruri_regular_subset_ttf_len;

// c.f. http://ncl.sakura.ne.jp/doc/ja/comp/freetype-memo.html

FT_Library library;
FT_Face face;
void InitFreeType() {
  int err;
  unsigned char* font = koruri_regular_subset_ttf;
  const unsigned int font_len = koruri_regular_subset_ttf_len;
  err = FT_Init_FreeType(&library);
  if (err) {
    Print("Failed to init FreeType");
    exit(1);
  }
  err = FT_New_Memory_Face(library, font, font_len, 0, &face);
  if (err) {
    Print("Failed to load font face");
    exit(1);
  }
  err = FT_Set_Pixel_Sizes(face, 32, 32);
  if (err) {
    Print("Failed to set pixel sizes");
    exit(1);
  }
}

int CHNLIB_UTF8_GetCharacterType(uint8_t c) {
  //[UTF-8]
  // UTF-8文字列中の1バイトcが、UTF-8文字列中でどのような役割を持つのかを返す。
  // 0:マルチバイト文字の後続バイト
  // 1:1バイト文字
  // 2以上の数値n:nバイト文字の最初のバイト。後続のn個のマルチバイト後続バイトと組み合わせて一つの文字を示す。
  //判断不能な場合は-1を返す。
  if (((c >> 6) & 3) == 2) {
    //マルチバイト後続バイト
    // 10xxxxxx
    return 0;
  } else if (((c >> 7) & 1) == 0) {
    // 1Byte
    // 7bit
    // 0xxxxxxx
    return 1;
  } else if (((c >> 5) & 7) == 6) {
    // 2Byte
    // 11bit
    // 110xxxxx
    return 2;
  } else if (((c >> 4) & 15) == 14) {
    // 3Byte
    // 16bit
    // 1110xxxx
    return 3;
  } else if (((c >> 3) & 31) == 30) {
    // 4Byte
    // 21bit
    // 11110xxx
    return 4;
  }

  return -1;
}

uint32_t CHNLIB_UTF8_GetNextUnicodeOfCharacter(const char ss[],
                                               const char** next) {
  //[UTF-8]
  // sをUTF-8文字列として解釈し、その文字列の最初の文字のUnicodeを返す。
  //また、nextがNULLでないとき、*nextに、この文字列中で次の文字にあたる部分へのポインタを格納する。
  // s==NULLの時、*nextの値は変更されない。
  //次の文字が存在しない場合は、*nextはs中の終端文字へのポインタとなる。また、戻り値は0となる。
  //無効な（マルチバイトの一部が欠けているような）文字は無視する。
  int i, n, mbsize;
  uint32_t unicode;
  const uint8_t* s = (uint8_t*)ss;

  if (s == NULL) {
    return 0;
  }

  unicode = 0;
  mbsize = 0;
  for (i = 0; s[i] != '\0'; i++) {
    n = CHNLIB_UTF8_GetCharacterType(s[i]);
    switch (n) {
      case -1:
        //無効なバイト
        mbsize = 0;
        break;
      case 0:
        //マルチバイト後続バイト
        if (mbsize > 0) {
          mbsize--;
          unicode <<= 6;
          unicode |= (0x3f & s[i]);
          if (mbsize == 0) {
            return unicode;
          }
        }
        break;
      case 1:
        // 1バイト文字
        if (next != NULL) {
          *next = (char*)(s + 1);
        }
        return s[i];
      default:
        // nバイト文字の最初のバイト
        unicode = s[i] & ((1 << (7 - n)) - 1);
        mbsize = n - 1;
        if (next != NULL) {
          *next = (char*)(s + n);
        }
        break;
    }
  }
  if (next != NULL) {
    *next = (char*)&s[i];
  }
  return 0;
}

void DrawChar(uint32_t* bmp,
              int w,
              int px,
              int py,
              uint32_t color,
              int unicode) {
  int err;
  err = FT_Load_Char(face, unicode, 0);
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
  int y_adjust = (32 - bm->rows) / 2;
  int x_adjust = (32 - (bm->pitch) * 8) / 2;
  for (row = 0; row < bm->rows; row++) {
    int x = 0;
    for (col = 0; col < bm->pitch; col++) {
      c = bm->buffer[bm->pitch * row + col];
      for (bit = 7; bit >= 0; bit--) {
        if ((c >> bit) & 1) {
          bmp[(py + row + y_adjust) * w + (px + x + x_adjust)] = color;
        }
        x++;
      }
    }
  }
}

void DrawFirstChar(uint32_t* bmp,
                   int w,
                   int x,
                   int y,
                   uint32_t col,
                   const char* s) {
  uint32_t unicode = CHNLIB_UTF8_GetNextUnicodeOfCharacter(s, NULL);
  DrawChar(bmp, w, x, y, col, unicode);
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
  // TODO: Implement free
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

void* memchr(const void* a, int b, unsigned long c) {
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

int setjmp(long long* buf) {
  // NotImplemented(__FUNCTION__);
  return __builtin_setjmp((void**)buf);
}
void longjmp() {
  NotImplemented(__FUNCTION__);
}
