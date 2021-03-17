#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]
#![allow(clippy::zero_ptr)]
#![deny(unused_must_use)]

use core::fmt;
use core::iter::Iterator;
use core::mem;
use core::panic::PanicInfo;

pub mod debug_exit;
pub mod efi;
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

struct MemoryMapHolder {
    memory_map_buffer: [u8; efi::MEMORY_MAP_BUFFER_SIZE],
    memory_map_size: efi::EFINativeUInt,
    map_key: efi::EFINativeUInt,
    descriptor_size: efi::EFINativeUInt,
    descriptor_version: u32,
}
struct MemoryMapIterator<'a> {
    map: &'a MemoryMapHolder,
    ofs: efi::EFINativeUInt,
}
impl<'a> Iterator for MemoryMapIterator<'a> {
    type Item = &'a efi::EFIMemoryDescriptor;
    fn next(&mut self) -> Option<&'a efi::EFIMemoryDescriptor> {
        if self.ofs >= self.map.memory_map_size {
            None
        } else {
            let e: &efi::EFIMemoryDescriptor = unsafe {
                &*(self.map.memory_map_buffer.as_ptr().add(self.ofs as usize)
                    as *const efi::EFIMemoryDescriptor)
            };
            self.ofs += self.map.descriptor_size;
            Some(e)
        }
    }
}

impl MemoryMapHolder {
    fn new() -> MemoryMapHolder {
        MemoryMapHolder {
            memory_map_buffer: [0; efi::MEMORY_MAP_BUFFER_SIZE],
            memory_map_size: efi::MEMORY_MAP_BUFFER_SIZE as efi::EFINativeUInt,
            map_key: 0,
            descriptor_size: 0,
            descriptor_version: 0,
        }
    }
    fn iter(&self) -> MemoryMapIterator {
        MemoryMapIterator { map: &self, ofs: 0 }
    }
}

fn get_memory_map(
    efi_system_table: &efi::EFISystemTable,
    map: &mut MemoryMapHolder,
) -> efi::EFIStatus {
    (efi_system_table.boot_services.get_memory_map)(
        &mut map.memory_map_size,
        map.memory_map_buffer.as_mut_ptr(),
        &mut map.map_key,
        &mut map.descriptor_size,
        &mut map.descriptor_version,
    )
}

fn main_with_boot_services(
    image_handle: efi::EFIHandle,
    efi_system_table: &efi::EFISystemTable,
    memory_map: &mut MemoryMapHolder,
) -> fmt::Result {
    #[cfg(test)]
    test_main();

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

    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut serial_writer = serial::SerialConsoleWriter {};

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
    let status = get_memory_map(&efi_system_table, memory_map);
    assert_eq!(status, efi::EFIStatus::SUCCESS);
    writeln!(
        efi_writer,
        "memory_map_size: {}",
        memory_map.memory_map_size
    )?;
    writeln!(
        efi_writer,
        "descriptor_size: {}",
        memory_map.descriptor_size
    )?;
    writeln!(efi_writer, "map_key: {:X}", memory_map.map_key)?;

    let status =
        (efi_system_table.boot_services.exit_boot_services)(image_handle, memory_map.map_key);
    assert_eq!(status, efi::EFIStatus::SUCCESS);

    writeln!(serial_writer, "Exited from EFI Boot Services")?;

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
    for e in memory_map.iter() {
        if e.memory_type == efi::EFIMemoryType::CONVENTIONAL_MEMORY {
            writeln!(serial_writer, "{:#?}", e)?;
            total_conventional_memory_size += e.number_of_pages * 4096;
        }
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
