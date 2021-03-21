#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]
#![allow(clippy::zero_ptr)]
#![deny(unused_must_use)]

use core::fmt;
use core::mem;
use core::option::Option;
use core::panic::PanicInfo;
use memory_map_holder::MemoryMapHolder;

pub mod debug_exit;
pub mod efi;
pub mod efi_support;
pub mod memory_map_holder;
pub mod physical_page_allocator;
pub mod serial;
pub mod x86;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "PANIC!!!").unwrap();
    writeln!(writer, "{}", info).unwrap();

    loop {}
}
#[no_mangle]
pub extern "win64" fn efi_main(
    image_handle: efi::EFIHandle,
    efi_system_table: &efi::EFISystemTable,
) -> ! {
    let mut memory_map = MemoryMapHolder::new();
    main_with_boot_services(image_handle, efi_system_table, &mut memory_map).unwrap();
    main_without_boot_services(&mut memory_map).unwrap();
    loop {
        x86::hlt();
    }
}

fn main_with_boot_services(
    image_handle: efi::EFIHandle,
    efi_system_table: &efi::EFISystemTable,
    memory_map: &mut MemoryMapHolder,
) -> fmt::Result {
    #[cfg(test)]
    test_main();

    serial::com_initialize(serial::IO_ADDR_COM2);

    assert_eq!(
        efi_system_table.header.signature,
        efi::EFI_SYSTEM_TABLE_SIGNATURE
    );

    use core::fmt::Write;
    (efi_system_table.con_out.clear_screen)(efi_system_table.con_out);
    let mut efi_writer = efi::EFISimpleTextOutputProtocolWriter {
        protocol: &efi_system_table.con_out,
    };
    writeln!(efi_writer, "Loading liumOS...")?;
    writeln!(efi_writer, "{:#p}", &efi_system_table)?;

    let mut graphics_output_protocol: *mut efi::EFIGraphicsOutputProtocol =
        0 as *mut efi::EFIGraphicsOutputProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .locate_protocol
            .locate_graphics_output_protocol)(
            &efi::EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
            0 as *mut efi::EFIVoid,
            &mut graphics_output_protocol,
        );
        assert_eq!(status, efi::EFIStatus::SUCCESS);
    }

    unsafe {
        let vram: *mut u32 = (*(*graphics_output_protocol).mode).frame_buffer_base as *mut u32;
        let pixels_per_scan_line = (*(*(*graphics_output_protocol).mode).info).pixels_per_scan_line;
        let xsize = (*(*(*graphics_output_protocol).mode).info).horizontal_resolution;
        let ysize = (*(*(*graphics_output_protocol).mode).info).vertical_resolution;
        for y in 0..ysize {
            for x in 0..xsize {
                *vram.add((pixels_per_scan_line * y + x) as usize) = (y << 8) | x;
            }
        }
    }
    writeln!(efi_writer, "VRAM acquired")?;

    let mut loaded_image_protocol: *mut efi::EFILoadedImageProtocol =
        0 as *mut efi::EFILoadedImageProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .handle_protocol
            .handle_loaded_image_protocol)(
            image_handle,
            &efi::EFI_LOADED_IMAGE_PROTOCOL_GUID,
            &mut loaded_image_protocol,
        );
        assert_eq!(status, efi::EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got LoadedImageProtocol. Revision: {:#X} system_table: {:#p}",
            (*loaded_image_protocol).revision,
            (*loaded_image_protocol).system_table
        )?;
    }

    let mut simple_file_system_protocol: *mut efi::EFISimpleFileSystemProtocol =
        0 as *mut efi::EFISimpleFileSystemProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .handle_protocol
            .handle_simple_file_system_protocol)(
            (*loaded_image_protocol).device_handle,
            &efi::EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
            &mut simple_file_system_protocol,
        );
        assert_eq!(status, efi::EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got SimpleFileSystemProtocol. revision: {:#X}",
            (*simple_file_system_protocol).revision
        )?;
    }

    let mut root_file: *mut efi::EFIFileProtocol = 0 as *mut efi::EFIFileProtocol;
    unsafe {
        let status = ((*simple_file_system_protocol).open_volume)(
            simple_file_system_protocol,
            &mut root_file,
        );
        assert_eq!(status, efi::EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got FileProtocol of the root file. revision: {:#X}",
            (*root_file).revision
        )?;
    }

    let mut root_fs_info: efi::EFIFileSystemInfo = efi::EFIFileSystemInfo::default();
    let mut root_fs_info_size: efi::EFINativeUInt = mem::size_of::<efi::EFIFileSystemInfo>();
    unsafe {
        let status = ((*root_file).get_info)(
            root_file,
            &efi::EFI_FILE_SYSTEM_INFO_GUID,
            &mut root_fs_info_size,
            &mut root_fs_info,
        );
        assert_eq!(status, efi::EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got root fs. volume label: {}",
            efi::CStrPtr16::from_ptr(root_fs_info.volume_label.as_ptr())
        )?;
    }

    // List all files under root dir
    loop {
        let mut file_info: efi::EFIFileInfo = efi::EFIFileInfo::default();
        let mut file_info_size;
        unsafe {
            file_info_size = mem::size_of::<efi::EFIFileInfo>();
            let status = ((*root_file).read)(root_file, &mut file_info_size, &mut file_info);
            assert_eq!(status, efi::EFIStatus::SUCCESS);
            if file_info_size == 0 {
                break;
            }
            writeln!(
                efi_writer,
                "FILE: {}",
                efi::CStrPtr16::from_ptr(file_info.file_name.as_ptr())
            )?;
        }
    }

    // Get a memory map and exit boot services
    efi_support::exit_from_efi_boot_services(image_handle, &efi_system_table, memory_map);

    Ok(())
}

fn main_without_boot_services(memory_map: &mut MemoryMapHolder) -> fmt::Result {
    use core::fmt::Write;

    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut serial_writer = serial::SerialConsoleWriter {};
    writeln!(serial_writer, "Entring main_without_boot_services...")?;
    let cr3: &mut x86::PML4 = unsafe { &mut *x86::read_cr3() };
    writeln!(serial_writer, "cr3 = {:#p}", x86::read_cr3())?;
    writeln!(serial_writer, "{}", &cr3)?;
    let pdpt = x86::get_pdpt(&cr3.entry[0]);
    writeln!(serial_writer, "{}", pdpt)?;
    let pd = x86::get_pd(&pdpt.entry[0]);
    writeln!(serial_writer, "{}", pd)?;

    let mut total_conventional_memory_size = 0;
    let mut prev_header_holder: Option<&mut physical_page_allocator::RegionHeader> = None;
    for e in memory_map.iter() {
        if e.memory_type != efi::EFIMemoryType::CONVENTIONAL_MEMORY {
            continue;
        }
        writeln!(serial_writer, "{:#?}", e)?;
        total_conventional_memory_size += e.number_of_pages * 4096;
        let header =
            unsafe { &mut *(e.physical_start as *mut physical_page_allocator::RegionHeader) };
        header.init(e.number_of_pages as usize);
        writeln!(serial_writer, "{:#?}", header)?;
        if let Some(prev) = prev_header_holder {
            header.set_next(prev)
        }
        prev_header_holder = Some(header);
    }
    writeln!(serial_writer, "{:?}", prev_header_holder)?;
    writeln!(serial_writer, "{:?}", prev_header_holder.iter())?;
    let prev_header = prev_header_holder.unwrap();
    for _ in 0..3 {
        let allocated_addr = prev_header.allocate_physical_page().unwrap();
        writeln!(serial_writer, "allocated: {:#X}", allocated_addr)?;
    }
    for _ in 0..3 {
        let allocated_addr = prev_header.allocate_physical_page().unwrap();
        writeln!(serial_writer, "allocated: {:#X}", allocated_addr)?;
        prev_header.free_physical_page(allocated_addr);
        writeln!(serial_writer, "freed: {:#X}", allocated_addr)?;
    }
    for h in prev_header.iter() {
        writeln!(serial_writer, "iter: {:?}", h)?;
    }
    writeln!(
        serial_writer,
        "total_conventional_memory_size: {}",
        total_conventional_memory_size
    )?;

    loop {
        x86::hlt();
    }
}

pub trait Testable {
    fn run(&self);
}

impl<T> Testable for T
where
    T: Fn(),
{
    fn run(&self) {
        use core::fmt::Write;
        serial::com_initialize(serial::IO_ADDR_COM2);
        let mut writer = serial::SerialConsoleWriter {};
        write!(writer, "{}...\t", core::any::type_name::<T>()).unwrap();
        self();
        writeln!(writer, "[PASS]").unwrap();
    }
}

#[test_case]
fn trivial_assertion() {
    assert_eq!(1, 1);
}

#[test_case]
fn strlen_char16_returns_valid_len() {
    assert_eq!(
        efi::strlen_char16(efi::CStrPtr16::from_ptr(
            // "hello" contains 5 chars
            "h\0e\0l\0l\0o\0\0\0".as_ptr().cast::<u16>()
        )),
        5
    );
}

#[cfg(test)]
#[start]
pub extern "win64" fn _start() -> ! {
    test_main();
    loop {}
}

#[cfg(test)]
fn test_runner(tests: &[&dyn Testable]) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "Running {} tests...", tests.len()).unwrap();
    for test in tests {
        test.run();
    }
    write!(writer, "Done!").unwrap();
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Success)
}
