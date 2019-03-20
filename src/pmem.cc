#include "liumos.h"

#include "pmem.h"

void PersistentMemoryManager::Init() {
  using namespace ACPI;
  assert(liumos->acpi.nfit);
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
    byte_size_ = spa_range->system_physical_address_range_length;
    free_pages_ = (byte_size_ >> kPageSizeExponent) - kGlobalHeaderPages;
    signature_ = kSignature;
    CLFlush(this);
    return;
  }
  assert(false);
}

uint64_t PersistentMemoryManager::Allocate(uint64_t bytesize) {
  uint64_t num_of_pages = ByteSizeToPageSize(bytesize);
  assert(num_of_pages + 1 < free_pages_);
  return 0;
}

void PersistentMemoryManager::Print() {
  PutStringAndHex("PMEM at", this);
  if (!IsValid()) {
    PutString("  INVALID. initialize will be required.\n");
    return;
  }
  PutString("  signature valid.\n");
  PutStringAndHex("  Size in byte", byte_size_);
  PutStringAndHex("  Free pages", free_pages_);
  PutStringAndHex("  Free size in byte", free_pages_ << kPageSizeExponent);
}
