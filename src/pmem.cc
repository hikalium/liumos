#include "liumos.h"

#include "pmem.h"

void PersistentObjectHeader::Init(uint64_t id, uint64_t num_of_pages) {
  signature_ = ~kSignature;
  CLFlush(&signature_);
  id_ = id;
  num_of_pages_ = num_of_pages;
  next_ = nullptr;
  signature_ = kSignature;
  CLFlush(this);
}

void PersistentObjectHeader::SetNext(PersistentObjectHeader* next) {
  assert(IsValid());
  next_ = next;
  CLFlush(&next_);
}

void PersistentObjectHeader::Print() {
  PutStringAndHex("Object #", id_);
  assert(IsValid());
  PutStringAndHex("  base", GetObjectBase<void*>());
  PutStringAndHex("  num_of_pages", num_of_pages_);
}

void PersistentMemoryManager::Init() {
  using namespace ACPI;
  assert(liumos->acpi.nfit);
  assert((reinterpret_cast<uint64_t>(this) & kPageAddrMask) == 0);
  NFIT& nfit = *liumos->acpi.nfit;
  for (auto& it : nfit) {
    if (it.type != NFIT::Entry::kTypeSPARangeStructure)
      continue;
    NFIT::SPARange* spa_range = reinterpret_cast<NFIT::SPARange*>(&it);
    if (!IsEqualGUID(
            reinterpret_cast<GUID*>(&spa_range->address_range_type_guid),
            &NFIT::SPARange::kByteAdressablePersistentMemory))
      continue;
    if (spa_range->system_physical_address_range_base !=
        reinterpret_cast<uint64_t>(this))
      continue;
    signature_ = ~kSignature;
    PutStringAndHex("SPARange #", spa_range->spa_range_structure_index);
    PutStringAndHex("  Base", spa_range->system_physical_address_range_base);
    PutStringAndHex("  Length",
                    spa_range->system_physical_address_range_length);
    page_idx_ = reinterpret_cast<uint64_t>(this) >> kPageSizeExponent;
    num_of_pages_ =
        spa_range->system_physical_address_range_length >> kPageSizeExponent;
    head_ = nullptr;
    last_persistent_process_info_ = nullptr;
    signature_ = kSignature;
    CLFlush(this, sizeof(*this));

    sentinel_.Init(0, 0);
    SetHead(&sentinel_);

    return;
  }
  assert(false);
}

PersistentProcessInfo* PersistentMemoryManager::AllocPersistentProcessInfo() {
  PersistentProcessInfo* info = AllocPages<PersistentProcessInfo*>(
      ByteSizeToPageSize(sizeof(PersistentProcessInfo)));
  last_persistent_process_info_ = info;
  CLFlush(&last_persistent_process_info_);
  return last_persistent_process_info_;
}

void PersistentMemoryManager::Print() {
  PutStringAndHex("PMEM at", this);
  if (!IsValid()) {
    PutString("  INVALID. initialize will be required.\n");
    return;
  }
  PutString("  signature valid.\n");
  PutStringAndHex("  Size in byte", num_of_pages_ << kPageSizeExponent);
  for (PersistentObjectHeader* h = head_; h; h = h->GetNext()) {
    h->Print();
  }
}

void PersistentMemoryManager::SetHead(PersistentObjectHeader* head) {
  head_ = head;
  CLFlush(&head_);
}
