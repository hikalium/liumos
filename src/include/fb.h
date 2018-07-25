#ifndef _FB_H_
#define _FB_H_

struct fb {
	unsigned long long base;
	unsigned long long size;
	unsigned int hr;
	unsigned int vr;
};

extern struct fb fb;

void init_fb(void);

#endif
