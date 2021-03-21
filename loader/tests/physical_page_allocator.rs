#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![reexport_test_harness_main = "test_main"]
#![test_runner(crate::test_runner)]

use core::panic::PanicInfo;
use loader::*;

static mut MEMORY_MAP: memory_map_holder::MemoryMapHolder =
    memory_map_holder::MemoryMapHolder::new();

#[test_case]
fn p98_phys_pages_are_allocatable() {
    // Initialize allocator and calculate total_conventional_memory_size
    let mut total_conventional_memory_pages = 0;
    let mut prev_header_holder: Option<&mut physical_page_allocator::RegionHeader> = None;
    for e in unsafe { MEMORY_MAP.iter() } {
        if e.memory_type != efi::EFIMemoryType::CONVENTIONAL_MEMORY {
            continue;
        }
        total_conventional_memory_pages += e.number_of_pages;
        let header =
            unsafe { &mut *(e.physical_start as *mut physical_page_allocator::RegionHeader) };
        header.init(e.number_of_pages as usize);
        if let Some(prev) = prev_header_holder {
            header.set_next(prev)
        }
        prev_header_holder = Some(header);
    }
    println!(
        "total_conventional_memory_pages: {}",
        total_conventional_memory_pages
    );
    println!("Trying to allocate all available physical pages...");
    let prev_header = prev_header_holder.unwrap();
    let mut total_pages_allocated = 0;
    loop {
        let allocated_page_holder = prev_header.allocate_physical_page();
        if allocated_page_holder.is_none() {
            break;
        }
        total_pages_allocated += 1;
        if total_pages_allocated % 1000 == 0 {
            println!("  {} pages allocated...", total_pages_allocated);
        }
    }
    println!("total_pages_allocated: {}", total_pages_allocated);
    let allocated_rate = 100 * total_pages_allocated / total_conventional_memory_pages;
    println!("allocated_rate: {}%", allocated_rate);
    assert!(98 <= allocated_rate && allocated_rate <= 100);
}

#[no_mangle]
pub extern "win64" fn efi_main(
    image_handle: efi::EFIHandle,
    efi_system_table: &efi::EFISystemTable,
) -> ! {
    // Setup prerequisites
    serial::com_initialize(serial::IO_ADDR_COM2);
    efi_support::exit_from_efi_boot_services(image_handle, &efi_system_table, unsafe {
        &mut MEMORY_MAP
    });

    // Run tests
    test_main();
    unreachable!();
}

//
// Test harness implementaion
//

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    serial::com_initialize(serial::IO_ADDR_COM2);
    println!("{}", info);
    println!("FAIL: panicked during the test.");
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Fail);
}

fn test_runner(tests: &[&dyn Testable]) -> ! {
    serial::com_initialize(serial::IO_ADDR_COM2);
    println!("Running {} tests in {}...", tests.len(), file!());
    for test in tests {
        test.run();
    }
    println!("Done!");
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Success)
}

pub trait Testable {
    fn run(&self);
}

impl<T> Testable for T
where
    T: Fn(),
{
    fn run(&self) {
        serial::com_initialize(serial::IO_ADDR_COM2);
        println!("---- Running {} ", core::any::type_name::<T>());
        self();
        println!("---- [PASS] {}", core::any::type_name::<T>());
    }
}
