#pragma once

#include "generic.h"

class EFIFile {
  friend class EFIFileManager;

 public:
  const uint8_t* GetBuf() { return buf_pages_; }
  uint64_t GetFileSize() { return file_size_; }
  const char* GetFileName() { return file_name_; }

 private:
  static constexpr int kFileNameSize = 16;
  char file_name_[kFileNameSize + 1];
  uint64_t file_size_;
  uint8_t* buf_pages_;
};
