#include "efi.h"
struct EFI_SYSTEM_TABLE *_system_table;

void EFIInit(struct EFI_SYSTEM_TABLE *system_table)
{
	_system_table = system_table;
}

void EFIPutString(unsigned short *s)
{
	_system_table->ConOut->OutputString(_system_table->ConOut, s);
}

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *system_table) {
  EFIInit(system_table);
  EFIPutString(L"Hello, liumOS world!...\r\n");
  while (1) {
  };
}
