#include "liumos.h"

ACPI::RSDT* ACPI::rsdt;
ACPI::NFIT* ACPI::nfit;
ACPI::MADT* ACPI::madt;
ACPI::HPET* ACPI::hpet;
ACPI::SRAT* ACPI::srat;

void ACPI::DetectTables() {
  assert(rsdt);
  XSDT* xsdt = rsdt->xsdt;
  const int num_of_xsdt_entries = (xsdt->length - kDescriptionHeaderSize) >> 3;
  for (int i = 0; i < num_of_xsdt_entries; i++) {
    const char* signature = static_cast<const char*>(xsdt->entry[i]);
    if (strncmp(signature, "NFIT", 4) == 0)
      nfit = static_cast<NFIT*>(xsdt->entry[i]);
    if (strncmp(signature, "HPET", 4) == 0)
      hpet = static_cast<HPET*>(xsdt->entry[i]);
    if (strncmp(signature, "APIC", 4) == 0)
      madt = static_cast<MADT*>(xsdt->entry[i]);
    if (strncmp(signature, "SRAT", 4) == 0)
      srat = static_cast<SRAT*>(xsdt->entry[i]);
  }
  if (!madt)
    Panic("MADT not found");
  if (!hpet)
    Panic("HPET not found");
}
