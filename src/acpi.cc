#include "liumos.h"

const GUID ACPI::NFIT::SPARange::kByteAdressablePersistentMemory = {
    0x66F0D379,
    0xB4F3,
    0x4074,
    {0xAC, 0x43, 0x0D, 0x33, 0x18, 0xB7, 0x8C, 0xDB}};

const GUID ACPI::NFIT::SPARange::kNVDIMMControlRegion = {
    0x92F701F6,
    0x13B4,
    0x405D,
    {0x91, 0x0B, 0x29, 0x93, 0x67, 0xE8, 0x23, 0x4C}};

void ACPI::DetectTables() {
  assert(liumos->acpi.rsdt);
  XSDT* xsdt = liumos->acpi.rsdt->xsdt;
  const int num_of_xsdt_entries = (xsdt->length - kDescriptionHeaderSize) >> 3;
  for (int i = 0; i < num_of_xsdt_entries; i++) {
    const auto signature = xsdt->entry[i];
    if (memcmp(signature, "NFIT", 4) == 0)
      liumos->acpi.nfit = static_cast<NFIT*>(xsdt->entry[i]);
    if (memcmp(signature, "HPET", 4) == 0)
      liumos->acpi.hpet = static_cast<HPET*>(xsdt->entry[i]);
    if (memcmp(signature, "APIC", 4) == 0)
      liumos->acpi.madt = static_cast<MADT*>(xsdt->entry[i]);
    if (memcmp(signature, "SRAT", 4) == 0)
      liumos->acpi.srat = static_cast<SRAT*>(xsdt->entry[i]);
    if (memcmp(signature, "SLIT", 4) == 0)
      liumos->acpi.slit = static_cast<SLIT*>(xsdt->entry[i]);
    if (memcmp(signature, "FACP", 4) == 0)
      liumos->acpi.fadt = static_cast<FADT*>(xsdt->entry[i]);
  }
  if (!liumos->acpi.madt)
    Panic("MADT not found");
  if (!liumos->acpi.hpet)
    Panic("HPET not found");
}
