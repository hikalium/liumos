#include "corefunc.h"
#include "liumos.h"

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
static const GUID kFileSystemInfoGUID = {
    // EFI_FILE_SYSTEM_INFO_ID
    0x09576e93,
    0x6d3f,
    0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
static const GUID kLoadedImageProtocolGUID = {
    0x5B1B31A1,
    0x9562,
    0x11d2,
    {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

static void AssertSuccess(EFI::Status status, const char* msg) {
  if (status == EFI::Status::kSuccess)
    return;
  PutChar('\n');
  PutString("AssertSuccess Failed. Reason: ");
  if (status == EFI::Status::kBufferTooSmall) {
    PutString("Buffer Too Small\n");
  } else if (status == EFI::Status::kNotFound) {
    PutString("Not Found\n");
  } else {
    PutStringAndHex("status", static_cast<int>(status));
  }
  Panic(msg);
}

void EFI::ClearScreen() {
  system_table_->con_out->clear_screen(system_table_->con_out);
}

void EFI::PutWChar(wchar_t c) {
  wchar_t buf[2];
  buf[0] = c;
  buf[1] = 0;
  system_table_->con_out->output_string(system_table_->con_out, buf);
}

wchar_t EFI::GetChar() {
  EFI::InputKey key;
  while (1) {
    if (!system_table_->con_in->ReadKeyStroke(system_table_->con_in, &key))
      break;
  }
  return key.UnicodeChar;
}

void EFI::MemoryMap::Init() {
  uint32_t descriptor_version;
  bytes_used_ = sizeof(buf_);
  EFI::Status status =
      CoreFunc::GetEFI().system_table_->boot_services->GetMemoryMap(
          &bytes_used_, buf_, &key_, &descriptor_size_, &descriptor_version);
  if (status == Status::kBufferTooSmall) {
    PutStringAndHex("\nBuffer size needed", bytes_used_);
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
  for (int i = 0; i < (int)system_table_->number_of_table_entries; i++) {
    if (IsEqualGUID(guid, &system_table_->configuration_table[i].vendor_guid))
      return system_table_->configuration_table[i].vendor_table;
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
    status = system_table_->boot_services->ExitBootServices(image_handle,
                                                            map.GetKey());
  } while (status != EFI::Status::kSuccess);
  PutString(" done.\n");
}

EFI::FileProtocol* EFI::OpenFile(const wchar_t* path) {
  EFI::FileProtocol* file = nullptr;
  EFI::Status status = root_file_->Open(root_file_, &file, path, EFI::kRead, 0);
  if (status != EFI::Status::kSuccess || !file) {
    PutString("filename: ");
    while (path && *path) {
      PutChar(*path);
      path++;
    }
    AssertSuccess(status, "Failed to open file");
  }
  return file;
}

void EFI::ReadFileInfo(EFI::FileProtocol* file, EFI::FileInfo* info) {
  UINTN buf_size = sizeof(FileInfo);
  EFI::Status status = file->GetInfo(file, &kFileInfoGUID, &buf_size,
                                     reinterpret_cast<uint8_t*>(info));
  if (status != EFI::Status::kSuccess)
    Panic("Failed to read file info");
}

static void ReadFileSystemInfo(EFI::FileProtocol* file,
                               EFI::FileSystemInfo* info) {
  EFI::UINTN buf_size = sizeof(*info);
  EFI::Status status = file->GetInfo(file, &kFileSystemInfoGUID, &buf_size,
                                     reinterpret_cast<uint8_t*>(info));
  AssertSuccess(status, "Failed to read file system info");
}

void* EFI::AllocatePages(UINTN pages) {
  void* mem;
  Status status = system_table_->boot_services->AllocatePages(
      AllocateType::kAnyPages, MemoryType::kLoaderData, pages, &mem);
  if (status != EFI::Status::kSuccess)
    Panic("Failed to alloc pages");
  return mem;
}

void EFI::Init(Handle image_handle, SystemTable* system_table) {
  image_handle_ = image_handle;
  system_table_ = system_table;
  system_table_->boot_services->SetWatchdogTimer(0, 0, 0, nullptr);
  system_table_->boot_services->LocateProtocol(
      &kGraphicsOutputProtocolGUID, nullptr,
      (void**)&graphics_output_protocol_);
  liumos->acpi.rsdt = static_cast<ACPI::RSDT*>(
      EFI::GetConfigurationTableByUUID(&kACPITableGUID));
  assert(liumos->acpi.rsdt);
  // Open volume which we boot from
  LoadedImageProtocol* loaded_image_protocol;
  assert(system_table_->boot_services->HandleProtocol(
             image_handle, &kLoadedImageProtocolGUID,
             reinterpret_cast<void**>(&loaded_image_protocol)) ==
         Status::kSuccess);
  assert(system_table_->boot_services->HandleProtocol(
             loaded_image_protocol->device_handle, &kSimpleFileSystemProtocolGUID,
             reinterpret_cast<void**>(&simple_fs_)) ==
         Status::kSuccess);
  assert(simple_fs_);
  assert(simple_fs_->OpenVolume(simple_fs_, &root_file_) == Status::kSuccess);
}

static void PutWString(const wchar_t* s) {
  while (s && *s) {
    PutChar(*s);
    s++;
  }
}

void EFI::ListAllFiles() {
  // 13.5 File Protocol
  assert(root_file_);

  FileSystemInfo fsinfo;
  ReadFileSystemInfo(root_file_, &fsinfo);

  PutString("Volume label: ");
  PutWString(fsinfo.volume_label);
  PutChar('\n');

  PutString("Files in root:\n");

  for (;;) {
    FileInfo finfo;
    UINTN buf_size = sizeof(finfo);
    Status status = root_file_->Read(root_file_, &buf_size, &finfo);
    assert(status == Status::kSuccess);
    if (!buf_size)
      break;  // End of dir entry
    PutChar('+');
    PutWString(finfo.file_name);
    PutChar('\n');
  }
}
