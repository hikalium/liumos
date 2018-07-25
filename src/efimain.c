#include <efi.h>
#include <common.h>

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *SystemTable)
{
	efi_init(SystemTable);
	puts(L"Hello, liumOS world!...\r\n");
	while (TRUE);
}

