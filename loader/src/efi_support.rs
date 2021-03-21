use crate::efi::*;
use crate::memory_map_holder;
use crate::println;

pub fn exit_from_efi_boot_services(
    image_handle: EFIHandle,
    efi_system_table: &EFISystemTable,
    memory_map: &mut memory_map_holder::MemoryMapHolder,
) {
    // Get a memory map and exit boot services
    let status = memory_map_holder::get_memory_map(&efi_system_table, memory_map);
    assert_eq!(status, EFIStatus::SUCCESS);
    println!("memory_map_size: {}", memory_map.memory_map_size);
    println!("descriptor_size: {}", memory_map.descriptor_size);
    println!("map_key: {:X}", memory_map.map_key);

    let status =
        (efi_system_table.boot_services.exit_boot_services)(image_handle, memory_map.map_key);
    assert_eq!(status, EFIStatus::SUCCESS);
    println!("Exited from EFI Boot Services");
}
