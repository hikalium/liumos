#ifndef _FILE_H_
#define _FILE_H_

#include "graphics.h"

#define MAX_FILE_NAME_LEN	4
#define MAX_FILE_NUM	10
#define MAX_FILE_BUF	1024
#define FILE_INFO_BUF_SIZE	180

struct FILE {
	struct RECT rect;
	unsigned char is_highlight;
	unsigned short name[MAX_FILE_NAME_LEN];
};

extern struct FILE file_list[MAX_FILE_NUM];

unsigned long long get_file_size(struct EFI_FILE_PROTOCOL *file);
void safety_file_read(struct EFI_FILE_PROTOCOL *src, void *dst,
		      unsigned long long size);

#endif
