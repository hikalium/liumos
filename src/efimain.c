#include <stdint.h>
#include "efi.h"

struct EFI_SYSTEM_TABLE *_system_table;

void EFIInit(struct EFI_SYSTEM_TABLE *system_table)
{
	_system_table = system_table;
}

void EFIClearScreen()
{
  _system_table->con_out->clear_screen(_system_table->con_out);
}

void EFIPutString(wchar_t *s)
{
	_system_table->con_out->output_string(_system_table->con_out, s);
}

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *system_table) {
  EFIInit(system_table);
  EFIClearScreen();
  EFIPutString(L"Hello, liumOS world!...\r\n");
  while (1) {
  };
}
