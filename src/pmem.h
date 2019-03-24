#pragma once
#include "execution_context.h"
#include "generic.h"

class PersistentObjectHeader {
 public:
  bool IsValid() { return signature_ == kSignature; }
  void Init(uint64_t id, uint64_t num_of_pages_);
  PersistentObjectHeader* GetNext() { return next_; };
  void SetNext(PersistentObjectHeader* next);
  void Print();
  template <typename T>
  T GetObjectBase() {
    return reinterpret_cast<T>(reinterpret_cast<uint64_t>(this) +
                               sizeof(*this));
  }
  uint64_t GetID() { return id_; }
  uint64_t GetNumOfPages() { return num_of_pages_; }
  uint64_t GetByteSize() { return num_of_pages_ << kPageSizeExponent; }

 private:
  static constexpr uint64_t kSignature = 0x4F50534F6D75696CULL;
  uint64_t signature_;
  uint64_t id_;
  uint64_t num_of_pages_;
  PersistentObjectHeader* next_;
};

class PersistentMemoryManager {
 public:
  bool IsValid() { return signature_ == kSignature && head_; }
  template <typename T>
  T AllocPages(uint64_t num_of_pages_requested) {
    assert(IsValid());
    const uint64_t next_page_idx =
        (head_->GetObjectBase<uint64_t>() >> kPageSizeExponent) +
        head_->GetNumOfPages();
    assert(num_of_pages_requested + 1 + next_page_idx <=
           page_idx_ + num_of_pages_);
    PersistentObjectHeader* h = reinterpret_cast<PersistentObjectHeader*>(
        ((next_page_idx + 1) << kPageSizeExponent) - sizeof(*h));
    h->Init(head_->GetID() + 1, num_of_pages_requested);
    h->SetNext(head_);
    SetHead(h);
    return head_->GetObjectBase<T>();
  }
  PersistentProcessInfo* AllocPersistentProcessInfo();
  PersistentProcessInfo* GetLastPersistentProcessInfo() {
    return last_persistent_process_info_;
  };

  void Init();
  void Print();

 private:
  void SetHead(PersistentObjectHeader* head);
  static constexpr uint64_t kSignature = 0x4D50534F6D75696CULL;
  uint64_t page_idx_;
  uint64_t num_of_pages_;
  PersistentObjectHeader* head_;
  PersistentProcessInfo* last_persistent_process_info_;
  PersistentObjectHeader sentinel_;
  uint64_t signature_;
};
