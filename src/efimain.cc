#include "liumos.h"

extern "C" {

void efi_main(void* image_handle, EFISystemTable* system_table) {
  MainForBootProcessor(image_handle, system_table);
}
}
