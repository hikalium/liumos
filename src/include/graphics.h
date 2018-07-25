#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "efi.h"

struct RECT {
	unsigned int x, y;
	unsigned int w, h;
};

extern const struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL white;
extern const struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL yellow;

void draw_pixel(unsigned int x, unsigned int y,
		struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL color);
void draw_rect(struct RECT r, struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL c);
struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL get_pixel(unsigned int x, unsigned int y);
unsigned char is_in_rect(unsigned int x, unsigned int y, struct RECT r);
void blt(unsigned char img[], unsigned int img_width, unsigned int img_height);

#endif
