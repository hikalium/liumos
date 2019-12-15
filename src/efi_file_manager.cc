#include "efi_file_manager.h"

#include "corefunc.h"
#include "efi.h"

void EFIFileManager::Load(EFIFile& dst, const wchar_t* file_name) {
  for (int i = 0; i < EFIFile::kFileNameSize; i++) {
    dst.file_name_[i] = (char)file_name[i];
    if (!file_name[i])
      break;
  }
  dst.file_name_[EFIFile::kFileNameSize] = 0;
  EFI::FileProtocol* file = CoreFunc::GetEFI().OpenFile(file_name);
  EFI::FileInfo info;
  CoreFunc::GetEFI().ReadFileInfo(file, &info);
  EFI::UINTN buf_size = info.file_size;
  dst.buf_pages_ = reinterpret_cast<uint8_t*>(
      CoreFunc::GetEFI().AllocatePages(ByteSizeToPageSize(buf_size)));
  if (file->Read(file, &buf_size, dst.buf_pages_) != EFI::Status::kSuccess) {
    Panic("Read failed\n");
    return;
  }
  assert(buf_size == info.file_size);
  dst.file_size_ = info.file_size;
  return;
}
