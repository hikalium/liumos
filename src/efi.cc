#include "liumos.h"

EFISystemTable* _system_table;
EFIGraphicsOutputProtocol* efi_graphics_output_protocol;

bool IsEqualStringWithSize(const char* s1, const char* s2, int n) {
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i])
      return false;
  }
  return true;
}

void EFIClearScreen() {
  _system_table->con_out->clear_screen(_system_table->con_out);
}

void EFIPutString(const wchar_t* s) {
  _system_table->con_out->output_string(_system_table->con_out, s);
}

void EFIPutChar(wchar_t c) {
  wchar_t buf[2];
  buf[0] = c;
  buf[1] = 0;
  _system_table->con_out->output_string(_system_table->con_out, buf);
}

void EFIPutCString(const char* s) {
  while (*s) {
    EFIPutChar(*s);
    s++;
  }
}

void EFIPutnCString(const char* s, int n) {
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

void EFIMemoryMap::Init() {
  uint32_t descriptor_version;
  bytes_used_ = sizeof(buf_);
  EFIStatus status = _system_table->boot_services->GetMemoryMap(
      &bytes_used_, buf_, &key_, &descriptor_size_, &descriptor_version);
  if (status != EFIStatus::kSuccess)
    Panic("Failed to get memory map");
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

void EFIPrintStringAndHex(const wchar_t* s, uint64_t value) {
  EFIPutString(s);
  EFIPutString(L": 0x");
  EFIPrintHex64(value);
  EFIPutString(L"\r\n");
}

void EFIMemoryDescriptor::Print() const {
  PutString("0x");
  PutHex64ZeroFilled(physical_start);
  PutString(" - 0x");
  PutHex64ZeroFilled(physical_start + (number_of_pages << 12));
  PutString(" Type 0x");
  PutHex64(static_cast<uint64_t>(type));
  PutString(" Virt 0x");
  PutHex64(virtual_start);
  PutString(" Attr 0x");
  PutHex64(attribute);
  PutString(" (0x");
  PutHex64(number_of_pages);
  PutString(" pages)");
}

void EFIMemoryMap::Print() {
  PutStringAndHex("Map entries", GetNumberOfEntries());
  for (int i = 0; i < GetNumberOfEntries(); i++) {
    const EFIMemoryDescriptor* desc = GetDescriptor(i);
    if (desc->type != EFIMemoryType::kConventionalMemory)
      continue;
    desc->Print();
    PutString("\n");
  }
}

void* EFIGetConfigurationTableByUUID(const GUID* guid) {
  for (int i = 0; i < (int)_system_table->number_of_table_entries; i++) {
    if (IsEqualGUID(guid, &_system_table->configuration_table[i].vendor_guid))
      return _system_table->configuration_table[i].vendor_table;
  }
  return nullptr;
}

void EFIGetMemoryMapAndExitBootServices(EFIHandle image_handle,
                                        EFIMemoryMap& map) {
  EFIStatus status;
  PutString("Trying to exit EFI boot services..");
  do {
    PutString(".");
    map.Init();
    status = _system_table->boot_services->ExitBootServices(image_handle,
                                                            map.GetKey());
  } while (status != EFIStatus::kSuccess);
  PutString(" done.\n");
}
