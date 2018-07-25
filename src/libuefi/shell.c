#include <efi.h>
#include <common.h>
#include <file.h>
#include <graphics.h>
#include <shell.h>
#include <gui.h>

#define MAX_COMMAND_LEN	100
#define MAX_IMG_BUF	4194304	/* 4MB */

unsigned char img_buf[MAX_IMG_BUF];

void dialogue_get_filename(int idx)
{
	int i;

	ST->ConOut->ClearScreen(ST->ConOut);

	puts(L"New File Name: ");
	for (i = 0; i < MAX_FILE_NAME_LEN; i++) {
		file_list[idx].name[i] = getc();
		if (file_list[idx].name[i] != L'\r')
			putc(file_list[idx].name[i]);
		else
			break;
	}
	file_list[idx].name[i] = L'\0';
}

void pstat(void)
{
	unsigned long long status;
	struct EFI_SIMPLE_POINTER_STATE s;
	unsigned long long waitidx;

	SPP->Reset(SPP, FALSE);

	while (1) {
		ST->BootServices->WaitForEvent(1, &(SPP->WaitForInput),
					       &waitidx);
		status = SPP->GetState(SPP, &s);
		if (!status) {
			puth(s.RelativeMovementX, 8);
			puts(L" ");
			puth(s.RelativeMovementY, 8);
			puts(L" ");
			puth(s.RelativeMovementZ, 8);
			puts(L" ");
			puth(s.LeftButton, 1);
			puts(L" ");
			puth(s.RightButton, 1);
			puts(L"\r\n");
		}
	}
}

int ls(void)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	unsigned long long buf_size;
	unsigned char file_buf[MAX_FILE_BUF];
	struct EFI_FILE_INFO *file_info;
	int idx = 0;
	int file_num;

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	while (1) {
		buf_size = MAX_FILE_BUF;
		status = root->Read(root, &buf_size, (void *)file_buf);
		assert(status, L"root->Read");
		if (!buf_size) break;

		file_info = (struct EFI_FILE_INFO *)file_buf;
		strncpy(file_list[idx].name, file_info->FileName,
			MAX_FILE_NAME_LEN - 1);
		file_list[idx].name[MAX_FILE_NAME_LEN - 1] = L'\0';
		puts(file_list[idx].name);
		puts(L" ");

		idx++;
	}
	puts(L"\r\n");
	file_num = idx;

	root->Close(root);

	return file_num;
}

void cat(unsigned short *file_name)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name, EFI_FILE_MODE_READ, 0);
	assert(status, L"root->Open");

	status = file->Read(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Read");

	puts(file_buf);

	file->Close(file);
	root->Close(root);
}

void edit(unsigned short *file_name)
{
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned long long buf_size = MAX_FILE_BUF;
	unsigned short file_buf[MAX_FILE_BUF / 2];
	int i = 0;
	unsigned short ch;

	ST->ConOut->ClearScreen(ST->ConOut);

	while (TRUE) {
		ch = getc();

		if (ch == SC_ESC)
			break;

		putc(ch);
		file_buf[i++] = ch;

		if (ch == L'\r') {
			putc(L'\n');
			file_buf[i++] = L'\n';
		}
	}
	file_buf[i] = L'\0';

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"SFSP->OpenVolume");

	status = root->Open(root, &file, file_name,
			    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | \
			    EFI_FILE_MODE_CREATE, 0);
	assert(status, L"root->Open");

	status = file->Write(file, &buf_size, (void *)file_buf);
	assert(status, L"file->Write");

	file->Flush(file);

	file->Close(file);
	root->Close(root);
}

void view(unsigned short *img_name)
{
	unsigned long long buf_size = MAX_IMG_BUF;
	unsigned long long status;
	struct EFI_FILE_PROTOCOL *root;
	struct EFI_FILE_PROTOCOL *file;
	unsigned int vr = GOP->Mode->Info->VerticalResolution;
	unsigned int hr = GOP->Mode->Info->HorizontalResolution;

	status = SFSP->OpenVolume(SFSP, &root);
	assert(status, L"error: SFSP->OpenVolume");

	status = root->Open(root, &file, img_name, EFI_FILE_MODE_READ,
			    EFI_FILE_READ_ONLY);
	assert(status, L"error: root->Open");

	status = file->Read(file, &buf_size, (void *)img_buf);
	if (check_warn_error(status, L"warning:file->Read"))
		blt(img_buf, hr, vr);

	while (getc() != SC_ESC);

	status = file->Close(file);
	status = root->Close(root);
}

void shell(void)
{
	unsigned short com[MAX_COMMAND_LEN];
	struct RECT r = {10, 10, 100, 200};
	unsigned char is_exit = FALSE;

	while (!is_exit) {
		puts(L"poiOS> ");
		if (gets(com, MAX_COMMAND_LEN) <= 0)
			continue;

		if (!strcmp(L"hello", com))
			puts(L"Hello UEFI!\r\n");
		else if (!strcmp(L"rect", com))
			draw_rect(r, white);
		else if (!strcmp(L"gui", com)) {
			gui();
			ST->ConOut->ClearScreen(ST->ConOut);
		} else if (!strcmp(L"pstat", com))
			pstat();
		else if (!strcmp(L"ls", com))
			ls();
		else if (!strcmp(L"cat", com))
			cat(L"abc");
		else if (!strcmp(L"edit", com))
			edit(L"abc");
		else if (!strcmp(L"view", com)) {
			view(L"img");
			ST->ConOut->ClearScreen(ST->ConOut);
		} else if (!strcmp(L"exit", com))
			is_exit = TRUE;
		else
			puts(L"Command not found.\r\n");
	}
}
