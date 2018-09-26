#include "efi.h"

#define MEMORY_MAP_ENTRIES 64

EFISystemTable* _system_table;
EFIMemoryDescriptor memory_map[MEMORY_MAP_ENTRIES];
EFI_UINTN memory_map_used;

bool IsEqualStringWithSize(const char* s1, const char* s2, int n) {
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i])
      return false;
  }
  return true;
}

void EFIInit(struct EFI_SYSTEM_TABLE* system_table) {
  _system_table = system_table;
}

void EFIClearScreen() {
  _system_table->con_out->clear_screen(_system_table->con_out);
}

void EFIPutString(wchar_t* s) {
  _system_table->con_out->output_string(_system_table->con_out, s);
}

void EFIPutChar(wchar_t c) {
  wchar_t buf[2];
  buf[0] = c;
  buf[1] = 0;
  _system_table->con_out->output_string(_system_table->con_out, buf);
}

void EFIPutCString(char* s) {
  while (*s) {
    EFIPutChar(*s);
    s++;
  }
}

void EFIPutnCString(char* s, int n) {
  wchar_t buf[2];
  buf[1] = 0;
  for (int i = 0; i < n; i++) {
    buf[0] = s[i];
    _system_table->con_out->output_string(_system_table->con_out, buf);
  }
}

wchar_t EFIGetChar() {
  EFIInputKey key;
  while (1) {
    if (!_system_table->con_in->ReadKeyStroke(_system_table->con_in, &key))
      break;
  }
  return key.UnicodeChar;
}

void EFIGetMemoryMap() {
  EFI_UINTN map_key;
  EFI_UINTN desc_size;
  uint32_t desc_version;
  EFI_UINTN memory_map_byte_size =
      sizeof(EFIMemoryDescriptor) * MEMORY_MAP_ENTRIES;
  _system_table->boot_services->GetMemoryMap(
      &memory_map_byte_size, memory_map, &map_key, &desc_size, &desc_version);
  memory_map_used = memory_map_byte_size / desc_size;
}

void EFIPrintHex64(uint64_t value) {
  int i;
  wchar_t s[2];
  s[1] = 0;
  for (i = 15; i > 0; i--) {
    if ((value >> (4 * i)) & 0xF)
      break;
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

void EFIPrintStringAndHex(wchar_t* s, uint64_t value) {
  EFIPutString(s);
  EFIPutString(L": 0x");
  EFIPrintHex64(value);
  EFIPutString(L"\r\n");
}

void EFIPrintMemoryDescriptor(EFIMemoryDescriptor* desc) {
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

void EFIPrintMemoryMap() {
  EFIPrintStringAndHex(L"Map entries: ", memory_map_used);
  for (int i = 0; i < memory_map_used; i++) {
    EFIPutString(L"#");
    EFIPrintHex64(i);
    EFIPrintMemoryDescriptor(&memory_map[i]);
    EFIPutString(L"\r\n");
  }
  EFIPutString(L"NV Map entries: \r\n");
  for (int i = 0; i < memory_map_used; i++) {
    if (!(memory_map[i].attribute & EFI_MEMORY_NV))
      continue;
    EFIPutString(L"#");
    EFIPrintHex64(i);
    EFIPrintMemoryDescriptor(&memory_map[i]);
    EFIPutString(L"\r\n");
  }
}

void* EFIGetConfigurationTableByUUID(const GUID* guid) {
  for (int i = 0; i < _system_table->number_of_table_entries; i++) {
    if (IsEqualGUID(guid, &_system_table->configuration_table[i].vendor_guid))
      return _system_table->configuration_table[i].vendor_table;
  }
  return NULL;
}
