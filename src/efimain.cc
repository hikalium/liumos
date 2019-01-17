#include "liumos.h"

extern "C" {

void efi_main(void* image_handle, EFI::SystemTable* system_table) {
  MainForBootProcessor(image_handle, system_table);
}
}
