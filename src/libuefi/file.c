#include <efi.h>
#include <common.h>
#include <file.h>

#define SAFETY_READ_UNIT	16384	/* 16KB */

struct FILE file_list[MAX_FILE_NUM];

unsigned long long get_file_size(struct EFI_FILE_PROTOCOL *file)
{
	unsigned long long status;
	unsigned long long fi_size = FILE_INFO_BUF_SIZE;
	unsigned long long fi_buf[FILE_INFO_BUF_SIZE];
	struct EFI_FILE_INFO *fi_ptr;

	status = file->GetInfo(file, &fi_guid, &fi_size, fi_buf);
	if (!check_warn_error(status, L"file->GetInfo failure."))
		return 0;

	fi_ptr = (struct EFI_FILE_INFO *)fi_buf;

	return fi_ptr->FileSize;
}

void safety_file_read(struct EFI_FILE_PROTOCOL *src, void *dst,
		      unsigned long long size)
{
	unsigned long long status;

	unsigned char *d = dst;
	while (size > SAFETY_READ_UNIT) {
		unsigned long long unit = SAFETY_READ_UNIT;
		status = src->Read(src, &unit, (void *)d);
		assert(status, L"safety_read");
		d += unit;
		size -= unit;
	}

	while (size > 0) {
		unsigned long long tmp_size = size;
		status = src->Read(src, &tmp_size, (void *)d);
		assert(status, L"safety_read");
		size -= tmp_size;
	}
}
