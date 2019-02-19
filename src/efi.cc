#include "liumos.h"

EFI::SystemTable* EFI::system_table;
EFI::GraphicsOutputProtocol* EFI::graphics_output_protocol;
EFI::SimpleFileSystemProtocol* EFI::simple_fs;

static EFI::FileProtocol* root_file;

static const GUID kACPITableGUID = {
    0x8868e871,
    0xe4f1,
    0x11d3,
    {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}};
static const GUID kGraphicsOutputProtocolGUID = {
    0x9042a9de,
    0x23dc,
    0x4a38,
    {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};
static const GUID kSimpleFileSystemProtocolGUID = {
    0x0964e5b22,
    0x6459,
    0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
static const GUID kFileInfoGUID = {
    0x09576e92,
    0x6d3f,
    0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

void EFI::ConOut::ClearScreen() {
  EFI::system_table->con_out->clear_screen(EFI::system_table->con_out);
}

void EFI::ConOut::PutChar(wchar_t c) {
  wchar_t buf[2];
  buf[0] = c;
  buf[1] = 0;
  EFI::system_table->con_out->output_string(EFI::system_table->con_out, buf);
}

wchar_t EFI::GetChar() {
  EFI::InputKey key;
  while (1) {
    if (!EFI::system_table->con_in->ReadKeyStroke(EFI::system_table->con_in,
                                                  &key))
      break;
  }
  return key.UnicodeChar;
}

void EFI::MemoryMap::Init() {
  uint32_t descriptor_version;
  bytes_used_ = sizeof(buf_);
  Status status = EFI::system_table->boot_services->GetMemoryMap(
      &bytes_used_, buf_, &key_, &descriptor_size_, &descriptor_version);
  if (status == Status::kBufferTooSmall) {
    PutStringAndHex("Buffer size needed", bytes_used_);
    Panic("Failed to get memory map (not enough buffer)");
  }
  if (status != Status::kSuccess) {
    PutStringAndHex("status", static_cast<int>(status));
    Panic("Failed to get memory map");
  }
}

void EFI::MemoryDescriptor::Print() const {
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

void EFI::MemoryMap::Print() {
  PutStringAndHex("Map entries", GetNumberOfEntries());
  for (int i = 0; i < GetNumberOfEntries(); i++) {
    const MemoryDescriptor* desc = GetDescriptor(i);
    if (desc->type != MemoryType::kConventionalMemory)
      continue;
    desc->Print();
    PutString("\n");
  }
}

void* EFI::GetConfigurationTableByUUID(const GUID* guid) {
  for (int i = 0; i < (int)EFI::system_table->number_of_table_entries; i++) {
    if (IsEqualGUID(guid,
                    &EFI::system_table->configuration_table[i].vendor_guid))
      return EFI::system_table->configuration_table[i].vendor_table;
  }
  return nullptr;
}

void EFI::GetMemoryMapAndExitBootServices(Handle image_handle,
                                          EFI::MemoryMap& map) {
  EFI::Status status;
  PutString("Trying to exit boot services..");
  do {
    PutString(".");
    map.Init();
    status = EFI::system_table->boot_services->ExitBootServices(image_handle,
                                                                map.GetKey());
  } while (status != EFI::Status::kSuccess);
  PutString(" done.\n");
}

EFI::FileProtocol* EFI::OpenFile(const wchar_t* path) {
  EFI::FileProtocol* file = nullptr;
  EFI::Status status = root_file->Open(root_file, &file, path, EFI::kRead, 0);
  if (status != EFI::Status::kSuccess || !file)
    Panic("Failed to open file");
  return file;
}

void EFI::ReadFileInfo(EFI::FileProtocol* file, EFI::FileInfo* info) {
  UINTN buf_size = sizeof(FileInfo);
  EFI::Status status = file->GetInfo(file, &kFileInfoGUID, &buf_size,
                                     reinterpret_cast<uint8_t*>(info));
  if (status != EFI::Status::kSuccess)
    Panic("Failed to read file info");
}

void* EFI::AllocatePages(UINTN pages) {
  void* mem;
  Status status = EFI::system_table->boot_services->AllocatePages(
      AllocateType::kAnyPages, MemoryType::kLoaderData, pages, &mem);
  if (status != EFI::Status::kSuccess)
    Panic("Failed to alloc pages");
  return mem;
}

void EFI::Init(SystemTable* system_table) {
  EFI::system_table = system_table;
  EFI::system_table->boot_services->SetWatchdogTimer(0, 0, 0, nullptr);
  EFI::system_table->boot_services->LocateProtocol(
      &kGraphicsOutputProtocolGUID, nullptr, (void**)&graphics_output_protocol);
  EFI::system_table->boot_services->LocateProtocol(
      &kSimpleFileSystemProtocolGUID, nullptr, (void**)&simple_fs);
  assert(simple_fs);
  ACPI::rsdt = static_cast<ACPI::RSDT*>(
      EFI::GetConfigurationTableByUUID(&kACPITableGUID));
  assert(ACPI::rsdt);
  EFI::Status status = EFI::simple_fs->OpenVolume(EFI::simple_fs, &root_file);
  assert(status == EFI::Status::kSuccess);
}
