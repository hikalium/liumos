#include <stdint.h>
#include "efi.h"

#define MEMORY_MAP_ENTRIES 64

EFISystemTable *_system_table;
EFIMemoryDescriptor memory_map[MEMORY_MAP_ENTRIES];
EFI_UINTN memory_map_used;

void EFIInit(struct EFI_SYSTEM_TABLE *system_table) {
  _system_table = system_table;
}

void EFIClearScreen() {
  _system_table->con_out->clear_screen(_system_table->con_out);
}

void EFIPutString(wchar_t *s) {
  _system_table->con_out->output_string(_system_table->con_out, s);
}

void EFIGetMemoryMap() {
  EFI_UINTN map_key;
  EFI_UINTN desc_size;
  uint32_t desc_version;
  EFI_UINTN memory_map_byte_size = sizeof(EFIMemoryDescriptor) * MEMORY_MAP_ENTRIES;
  _system_table->boot_services->GetMemoryMap(&memory_map_byte_size, memory_map,
                                             &map_key, &desc_size, &desc_version);
  memory_map_used = memory_map_byte_size / desc_size;
}

void EFIPrintHex64(uint64_t value) {
  int i;
  wchar_t s[2];
  s[1] = 0;
  for (i = 15; i > 0; i--) {
    if ((value >> (4 * i)) & 0xF) break;
  }
  for (; i >= 0; i--) {
    s[0] = (value >> (4 * i)) & 0xF;
    if (s[0] < 10)
      s[0] += '0';
    else
      s[0] += 'A' - 10;
    EFIPutString(s);
  }
}

void EFIPrintMemoryDescriptor(EFIMemoryDescriptor *desc)
{
    EFIPutString(L" Type ");
    EFIPrintHex64(desc->type);
    EFIPutString(L" Phys ");
    EFIPrintHex64(desc->physical_start);
    EFIPutString(L" Virt ");
    EFIPrintHex64(desc->virtual_start);
    EFIPutString(L" NumOfPages ");
    EFIPrintHex64(desc->number_of_pages);
    EFIPutString(L" Attr ");
    EFIPrintHex64(desc->attribute);
}

void EFIPrintMemoryMap()
{
  EFIPutString(L"Map entries: ");
  EFIPrintHex64(memory_map_used);
  EFIPutString(L"\r\n");
  for(int i = 0; i < memory_map_used; i++){
    EFIPutString(L"#");
    EFIPrintHex64(i);
    EFIPrintMemoryDescriptor(&memory_map[i]);
    EFIPutString(L"\r\n");
  }
  EFIPutString(L"NV Map entries: ");
  EFIPrintHex64(memory_map_used);
  EFIPutString(L"\r\n");
  for(int i = 0; i < memory_map_used; i++){
    if(!(memory_map[i].attribute & EFI_MEMORY_NV)) continue;
    EFIPutString(L"#");
    EFIPrintHex64(i);
    EFIPrintMemoryDescriptor(&memory_map[i]);
    EFIPutString(L"\r\n");
  }
}

void efi_main(void *ImageHandle, struct EFI_SYSTEM_TABLE *system_table) {
  EFIInit(system_table);
  EFIClearScreen();
  EFIPutString(L"Hello, liumOS world!...\r\n");
  EFIGetMemoryMap();
  EFIPrintMemoryMap();
  while (1) {
  };
}
