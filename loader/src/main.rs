#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]
#![allow(clippy::zero_ptr)]
#![deny(unused_must_use)]

use core::convert::TryInto;
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
pub extern "win64" fn efi_entry(
    image_handle: efi::EFIHandle,
    efi_system_table: &efi::EFISystemTable,
) -> ! {
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
    writeln!(efi_writer, "Loading liumOS...").unwrap();
    writeln!(efi_writer, "{:#p}", &efi_system_table).unwrap();

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
    writeln!(efi_writer, "VRAM acquired").unwrap();

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
        )
        .unwrap();
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
        )
        .unwrap();
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
        )
        .unwrap();
    }

    let mut root_fs_info: efi::EFIFileSystemInfo = efi::EFIFileSystemInfo::default();
    let mut root_fs_info_size: u64 = mem::size_of::<efi::EFIFileSystemInfo>().try_into().unwrap();
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
        )
        .unwrap();
    }

    // List all files under root dir
    loop {
        let mut file_info: efi::EFIFileInfo = efi::EFIFileInfo::default();
        let mut file_info_size;
        unsafe {
            file_info_size = mem::size_of::<efi::EFIFileInfo>().try_into().unwrap();
            let status = ((*root_file).read)(root_file, &mut file_info_size, &mut file_info);
            assert_eq!(status, efi::EFIStatus::SUCCESS);
            if file_info_size == 0 {
                break;
            }
            writeln!(
                efi_writer,
                "FILE: {}",
                efi::CStrPtr16::from_ptr(file_info.file_name.as_ptr())
            )
            .unwrap();
        }
    }

    // Get a memory map and exit boot services
    let mut memory_map_buffer: [u8; efi::MEMORY_MAP_BUFFER_SIZE] = [0; efi::MEMORY_MAP_BUFFER_SIZE];
    let mut memory_map_size: efi::EFINativeUInt = efi::MEMORY_MAP_BUFFER_SIZE as efi::EFINativeUInt;
    let mut map_key: efi::EFINativeUInt = 0;
    let mut descriptor_size: efi::EFINativeUInt = 0;
    let mut descriptor_version: u32 = 0;
    let status = (efi_system_table.boot_services.get_memory_map)(
        &mut memory_map_size,
        memory_map_buffer.as_mut_ptr(),
        &mut map_key,
        &mut descriptor_size,
        &mut descriptor_version,
    );
    assert_eq!(status, efi::EFIStatus::SUCCESS);
    writeln!(efi_writer, "memory_map_size: {}", memory_map_size).unwrap();
    writeln!(efi_writer, "descriptor_size: {}", descriptor_size).unwrap();
    writeln!(efi_writer, "map_key: {:X}", map_key).unwrap();

    let status = (efi_system_table.boot_services.exit_boot_services)(image_handle, map_key);
    assert_eq!(status, efi::EFIStatus::SUCCESS);

    writeln!(serial_writer, "Exited from EFI Boot Services").unwrap();

    let mut z: u8 = 0;
    loop {
        unsafe {
            let vram: *mut u32 = (*(*graphics_output_protocol).mode).frame_buffer_base as *mut u32;
            let pixels_per_scan_line =
                (*(*(*graphics_output_protocol).mode).info).pixels_per_scan_line;
            let xsize = (*(*(*graphics_output_protocol).mode).info).horizontal_resolution;
            let ysize = (*(*(*graphics_output_protocol).mode).info).vertical_resolution;
            for y in 0..ysize {
                for x in 0..xsize {
                    *vram.add((pixels_per_scan_line * y + x) as usize) =
                        (((z as u32) * 16) << 16) | (((y & 0xFF) * 16) << 8) | ((x & 0xFF) * 16);
                    asm!("pause");
                }
            }
        }
        z += 1;
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
