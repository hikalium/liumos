#include "corefunc.h"
#include "liumos.h"

void File::LoadFromEFISimpleFS(const wchar_t* file_name) {
  for (int i = 0; i < kFileNameSize; i++) {
    file_name_[i] = (char)file_name[i];
    if (!file_name[i])
      break;
  }
  file_name_[kFileNameSize] = 0;
  EFI::FileProtocol* file = CoreFunc::GetEFI().OpenFile(file_name);
  EFI::FileInfo info;
  CoreFunc::GetEFI().ReadFileInfo(file, &info);
  EFI::UINTN buf_size = info.file_size;
  buf_pages_ = reinterpret_cast<uint8_t*>(
      CoreFunc::GetEFI().AllocatePages(ByteSizeToPageSize(buf_size)));
  if (file->Read(file, &buf_size, buf_pages_) != EFI::Status::kSuccess) {
    PutString("Read failed\n");
    return;
  }
  assert(buf_size == info.file_size);
  file_size_ = info.file_size;
  return;
}
