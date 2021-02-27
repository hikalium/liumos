#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]
#![allow(clippy::zero_ptr)]
use core::convert::TryInto;
use core::fmt;
use core::mem;
use core::panic::PanicInfo;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct CStrPtr16 {
    ptr: *const u16,
}
impl CStrPtr16 {
    fn from_ptr(p: *const u16) -> CStrPtr16 {
        CStrPtr16 { ptr: p }
    }
}

pub fn strlen_char16(strp: CStrPtr16) -> usize {
    let mut len: usize = 0;
    unsafe {
        loop {
            if *strp.ptr.add(len) == 0 {
                break;
            }
            len += 1;
        }
    }
    len
}

impl fmt::Display for CStrPtr16 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        unsafe {
            let mut index = 0;
            loop {
                let c = *self.ptr.offset(index);
                if c == 0 {
                    break;
                }
                let bytes = c.to_be_bytes();
                let result = write!(f, "{}", bytes[1] as char);
                if result.is_err() {
                    return result;
                }
                index += 1;
            }
        }
        Result::Ok(())
    }
}

#[cfg(test)]
fn test_panic(info: &PanicInfo) {
    use core::fmt::Write;
    com_initialize(IO_ADDR_COM2);
    let mut writer = SerialConsoleWriter {};
    writeln!(writer, "[FAIL]");
    write!(writer, "{}", info);
    exit_qemu(QemuExitCode::Fail)
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    #[cfg(test)]
    test_panic(info);

    use core::fmt::Write;
    com_initialize(IO_ADDR_COM2);
    let mut writer = SerialConsoleWriter {};
    writeln!(writer, "PANIC!!!").unwrap();
    writeln!(writer, "{}", info).unwrap();

    loop {}
}

fn hlt() {
    unsafe { asm!("hlt") }
}

// https://doc.rust-lang.org/beta/unstable-book/library-features/asm.html

fn write_io_port(port: u16, data: u8) {
    unsafe {
        asm!("out dx, al",
                                in("al") data,
                                        in("dx") port)
    }
}
fn read_io_port(port: u16) -> u8 {
    let mut data: u8;
    unsafe {
        asm!("in al, dx",
                                    out("al") data,
                                            in("dx") port)
    }
    data
}

// const IO_ADDR_COM1: u16 = 0x3f8;
const IO_ADDR_COM2: u16 = 0x2f8;
fn com_initialize(base_io_addr: u16) {
    write_io_port(base_io_addr + 1, 0x00); // Disable all interrupts
    write_io_port(base_io_addr + 3, 0x80); // Enable DLAB (set baud rate divisor)
    const BAUD_DIVISOR: u16 = 0x0001; // baud rate = (115200 / BAUD_DIVISOR)
    write_io_port(base_io_addr, (BAUD_DIVISOR & 0xff).try_into().unwrap());
    write_io_port(base_io_addr + 1, (BAUD_DIVISOR >> 8).try_into().unwrap());
    write_io_port(base_io_addr + 3, 0x03); // 8 bits, no parity, one stop bit
    write_io_port(base_io_addr + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    write_io_port(base_io_addr + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

fn com_send_char(base_io_addr: u16, c: char) {
    while (read_io_port(base_io_addr + 5) & 0x20) == 0 {
        unsafe { asm!("pause") }
    }
    write_io_port(base_io_addr, c as u8)
}

fn com_send_str(base_io_addr: u16, s: &str) {
    let mut sc = s.chars();
    let slen = s.chars().count();
    for _ in 0..slen {
        com_send_char(base_io_addr, sc.next().unwrap());
    }
}

pub struct SerialConsoleWriter {}

impl SerialConsoleWriter {
    pub fn write_str(&mut self, s: &str) {
        com_send_str(IO_ADDR_COM2, s);
    }
}

impl fmt::Write for SerialConsoleWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        com_send_str(IO_ADDR_COM2, s);
        Ok(())
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum QemuExitCode {
    Success = 0x1, // QEMU will exit with status 3
    Fail = 0x2,    // QEMU will exit with status 5
}

pub fn exit_qemu(exit_code: QemuExitCode) -> ! {
    // https://github.com/qemu/qemu/blob/master/hw/misc/debugexit.c
    write_io_port(0xf4, exit_code as u8);
    loop {
        hlt();
    }
}

#[repr(C)]
pub struct EFITableHeader {
    signature: u64,
    revision: u32,
    header_size: u32,
    crc32: u32,
    reserved: u32,
}

#[repr(C)]
pub struct EFI_GUID {
    data0: u32,
    data1: u16,
    data2: u16,
    data3: [u8; 8],
}

pub type EFIHandle = u64;
pub type EFIVoid = u8;
pub type EFINativeUInt = u64;

const EFI_SYSTEM_TABLE_SIGNATURE: u64 = 0x5453595320494249;
const EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x9042a9de,
    data1: 0x23dc,
    data2: 0x4a38,
    data3: [0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a],
};
const EFI_LOADED_IMAGE_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x5B1B31A1,
    data1: 0x9562,
    data2: 0x11d2,
    data3: [0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B],
};
const EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x0964e5b22,
    data1: 0x6459,
    data2: 0x11d2,
    data3: [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
};
const EFI_FILE_SYSTEM_INFO_GUID: EFI_GUID = EFI_GUID {
    data0: 0x09576e93,
    data1: 0x6d3f,
    data2: 0x11d2,
    data3: [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
};
const MEMORY_MAP_BUFFER_SIZE: usize = 0x8000;

#[repr(i64)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum EFIStatus {
    SUCCESS = 0,
}

#[repr(C)]
pub struct EFISimpleTextOutputProtocol {
    reset: EFIHandle,
    output_string:
        extern "win64" fn(this: *const EFISimpleTextOutputProtocol, str: *const u16) -> EFIStatus,
    test_string: EFIHandle,
    query_mode: EFIHandle,
    set_mode: EFIHandle,
    set_attribute: EFIHandle,
    clear_screen: extern "win64" fn(this: *const EFISimpleTextOutputProtocol) -> EFIStatus,
}

pub union EFIBootServicesTableLocateProtocolVariants {
    locate_graphics_output_protocol: extern "win64" fn(
        protocol: *const EFI_GUID,
        registration: *const EFIVoid,
        interface: *mut *mut EFIGraphicsOutputProtocol,
    ) -> EFIStatus,
}
pub union EFIBootServicesTableHandleProtocolVariants {
    handle_loaded_image_protocol: extern "win64" fn(
        handle: EFIHandle,
        protocol: *const EFI_GUID,
        interface: *mut *mut EFILoadedImageProtocol,
    ) -> EFIStatus,
    handle_simple_file_system_protocol: extern "win64" fn(
        handle: EFIHandle,
        protocol: *const EFI_GUID,
        interface: *mut *mut EFISimpleFileSystemProtocol,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFIBootServicesTable {
    header: EFITableHeader,
    padding0: [u64; 4],
    get_memory_map: extern "win64" fn(
        memory_map_size: *mut EFINativeUInt,
        memory_map: *mut u8,
        map_key: *mut EFINativeUInt,
        descriptor_size: *mut EFINativeUInt,
        descriptor_version: *mut u32,
    ) -> EFIStatus,
    padding1: [u64; 11],
    handle_protocol: EFIBootServicesTableHandleProtocolVariants,
    padding2: [u64; 9],
    exit_boot_services:
        extern "win64" fn(image_handle: EFIHandle, map_key: EFINativeUInt) -> EFIStatus,
    padding3: [u64; 5],
    open_protocol: EFIHandle,
    padding4: [u64; 4],
    locate_protocol: EFIBootServicesTableLocateProtocolVariants,
}

#[repr(C)]
pub struct EFISystemTable<'a> {
    header: EFITableHeader,
    firmware_vendor: CStrPtr16,
    firmware_revision: u32,
    console_in_handle: EFIHandle,
    con_in: EFIHandle,
    console_out_handle: EFIHandle,
    con_out: &'a EFISimpleTextOutputProtocol,
    standard_error_handle: EFIHandle,
    std_err: &'a EFISimpleTextOutputProtocol,
    runtime_services: EFIHandle,
    boot_services: &'a EFIBootServicesTable,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocolPixelInfo {
    version: u32,
    horizontal_resolution: u32,
    vertical_resolution: u32,
    pixel_format: u32,
    red_mask: u32,
    green_mask: u32,
    blue_mask: u32,
    reserved_mask: u32,
    pixels_per_scan_line: u32,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocolMode<'a> {
    max_mode: u32,
    mode: u32,
    info: &'a EFIGraphicsOutputProtocolPixelInfo,
    size_of_info: u64,
    frame_buffer_base: u64,
    frame_buffer_size: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocol<'a> {
    reserved: [u64; 3],
    mode: &'a EFIGraphicsOutputProtocolMode<'a>,
}

#[repr(C)]
#[derive(Default)]
pub struct EFIFileSystemInfo {
    size: u64,
    readonly: u8,
    volume_size: u64,
    free_space: u64,
    block_size: u32,
    volume_label: [u16; 32],
}

#[repr(C)]
pub struct EFIFileProtocol {
    revision: u64,
    reserved0: [u64; 3],
    read: extern "win64" fn(
        this: *const EFIFileProtocol,
        buffer_size: &mut u64,
        buffer: &mut EFIFileInfo,
    ) -> EFIStatus,
    reserved1: [u64; 3],
    get_info: extern "win64" fn(
        this: *const EFIFileProtocol,
        information_type: *const EFI_GUID,
        buffer_size: &mut u64,
        buffer: &mut EFIFileSystemInfo,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFILoadedImageProtocol<'a> {
    revision: u32,
    parent_handle: EFIHandle,
    system_table: &'a EFISystemTable<'a>,
    device_handle: EFIHandle,
}

#[allow(dead_code)]
#[derive(Default)]
pub struct EFITime {
    year: u16,  // 1900 – 9999
    month: u8,  // 1 – 12
    day: u8,    // 1 – 31
    hour: u8,   // 0 – 23
    minute: u8, // 0 – 59
    second: u8, // 0 – 59
    pad1: u8,
    nanosecond: u32, // 0 – 999,999,999
    time_zone: u16,  // -1440 to 1440 or 2047
    daylight: u8,
    pad2: u8,
}

#[repr(C)]
#[derive(Default)]
pub struct EFIFileInfo {
    size: u64,
    file_size: u64,
    physical_size: u64,
    create_time: EFITime,
    last_access_time: EFITime,
    modification_time: EFITime,
    attr: u64,
    file_name: [u16; 32],
}

#[repr(C)]
pub struct FileProtocol {
    revision: u64,
    open: EFIHandle,
    close: EFIHandle,
    delete: EFIHandle,
    read: extern "win64" fn(
        this: *const EFISimpleFileSystemProtocol,
        buffer_size: &mut u64,
        buffer: &mut EFIFileInfo,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFISimpleFileSystemProtocol {
    revision: u64,
    open_volume: extern "win64" fn(
        this: *const EFISimpleFileSystemProtocol,
        root: *mut *mut EFIFileProtocol,
    ) -> EFIStatus,
}

pub struct EFISimpleTextOutputProtocolWriter<'a> {
    protocol: &'a EFISimpleTextOutputProtocol,
}

impl EFISimpleTextOutputProtocolWriter<'_> {
    pub fn write_char(&mut self, c: u8) {
        let cbuf: [u16; 2] = [c.into(), 0];
        (self.protocol.output_string)(self.protocol, cbuf.as_ptr());
    }
    pub fn write_str(&mut self, s: &str) {
        for c in s.bytes() {
            if c == b'\n' {
                self.write_char(b'\r');
            }
            self.write_char(c);
        }
    }
}

impl fmt::Write for EFISimpleTextOutputProtocolWriter<'_> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write_str(s);
        Ok(())
    }
}

#[no_mangle]
pub extern "win64" fn efi_entry(image_handle: EFIHandle, efi_system_table: &EFISystemTable) -> ! {
    #[cfg(test)]
    test_main();

    assert_eq!(
        efi_system_table.header.signature,
        EFI_SYSTEM_TABLE_SIGNATURE
    );

    use core::fmt::Write;
    (efi_system_table.con_out.clear_screen)(efi_system_table.con_out);
    let mut efi_writer = EFISimpleTextOutputProtocolWriter {
        protocol: &efi_system_table.con_out,
    };
    writeln!(efi_writer, "Loading liumOS...").unwrap();
    writeln!(efi_writer, "{:#p}", &efi_system_table).unwrap();

    com_initialize(IO_ADDR_COM2);
    let mut serial_writer = SerialConsoleWriter {};

    let mut graphics_output_protocol: *mut EFIGraphicsOutputProtocol =
        0 as *mut EFIGraphicsOutputProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .locate_protocol
            .locate_graphics_output_protocol)(
            &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
            0 as *mut EFIVoid,
            &mut graphics_output_protocol,
        );
        assert_eq!(status, EFIStatus::SUCCESS);
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

    let mut loaded_image_protocol: *mut EFILoadedImageProtocol = 0 as *mut EFILoadedImageProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .handle_protocol
            .handle_loaded_image_protocol)(
            image_handle,
            &EFI_LOADED_IMAGE_PROTOCOL_GUID,
            &mut loaded_image_protocol,
        );
        assert_eq!(status, EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got LoadedImageProtocol. Revision: {:#X} system_table: {:#p}",
            (*loaded_image_protocol).revision,
            (*loaded_image_protocol).system_table
        )
        .unwrap();
    }

    let mut simple_file_system_protocol: *mut EFISimpleFileSystemProtocol =
        0 as *mut EFISimpleFileSystemProtocol;
    unsafe {
        let status = (efi_system_table
            .boot_services
            .handle_protocol
            .handle_simple_file_system_protocol)(
            (*loaded_image_protocol).device_handle,
            &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
            &mut simple_file_system_protocol,
        );
        assert_eq!(status, EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got SimpleFileSystemProtocol. revision: {:#X}",
            (*simple_file_system_protocol).revision
        )
        .unwrap();
    }

    let mut root_file: *mut EFIFileProtocol = 0 as *mut EFIFileProtocol;
    unsafe {
        let status = ((*simple_file_system_protocol).open_volume)(
            simple_file_system_protocol,
            &mut root_file,
        );
        assert_eq!(status, EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got FileProtocol of the root file. revision: {:#X}",
            (*root_file).revision
        )
        .unwrap();
    }

    let mut root_fs_info: EFIFileSystemInfo = EFIFileSystemInfo::default();
    let mut root_fs_info_size: u64 = mem::size_of::<EFIFileSystemInfo>().try_into().unwrap();
    unsafe {
        let status = ((*root_file).get_info)(
            root_file,
            &EFI_FILE_SYSTEM_INFO_GUID,
            &mut root_fs_info_size,
            &mut root_fs_info,
        );
        assert_eq!(status, EFIStatus::SUCCESS);
        writeln!(
            efi_writer,
            "Got root fs. volume label: {}",
            CStrPtr16::from_ptr(root_fs_info.volume_label.as_ptr())
        )
        .unwrap();
    }

    // List all files under root dir
    loop {
        let mut file_info: EFIFileInfo = EFIFileInfo::default();
        let mut file_info_size;
        unsafe {
            file_info_size = mem::size_of::<EFIFileInfo>().try_into().unwrap();
            let status = ((*root_file).read)(root_file, &mut file_info_size, &mut file_info);
            assert_eq!(status, EFIStatus::SUCCESS);
            if file_info_size == 0 {
                break;
            }
            writeln!(
                efi_writer,
                "FILE: {}",
                CStrPtr16::from_ptr(file_info.file_name.as_ptr())
            )
            .unwrap();
        }
    }

    // Get a memory map and exit boot services
    let mut memory_map_buffer: [u8; MEMORY_MAP_BUFFER_SIZE] = [0; MEMORY_MAP_BUFFER_SIZE];
    let mut memory_map_size: EFINativeUInt = MEMORY_MAP_BUFFER_SIZE as EFINativeUInt;
    let mut map_key: EFINativeUInt = 0;
    let mut descriptor_size: EFINativeUInt = 0;
    let mut descriptor_version: u32 = 0;
    let status = (efi_system_table.boot_services.get_memory_map)(
        &mut memory_map_size,
        memory_map_buffer.as_mut_ptr(),
        &mut map_key,
        &mut descriptor_size,
        &mut descriptor_version,
    );
    assert_eq!(status, EFIStatus::SUCCESS);
    writeln!(efi_writer, "memory_map_size: {}", memory_map_size).unwrap();
    writeln!(efi_writer, "descriptor_size: {}", descriptor_size).unwrap();
    writeln!(efi_writer, "map_key: {:X}", map_key).unwrap();

    let status = (efi_system_table.boot_services.exit_boot_services)(image_handle, map_key);
    assert_eq!(status, EFIStatus::SUCCESS);

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
                        ((z as u32) * 16 << 16) | (((y & 0xFF) * 16) << 8) | ((x & 0xFF) * 16);
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
        com_initialize(IO_ADDR_COM2);
        let mut writer = SerialConsoleWriter {};
        write!(writer, "{}...\t", core::any::type_name::<T>()).unwrap();
        self();
        writeln!(writer, "[PASS]").unwrap();
    }
}

#[cfg(test)]
fn test_runner(tests: &[&dyn Testable]) -> ! {
    use core::fmt::Write;
    com_initialize(IO_ADDR_COM2);
    let mut writer = SerialConsoleWriter {};
    writeln!(writer, "Running {} tests...", tests.len());
    for test in tests {
        test.run();
    }
    write!(writer, "Done!");
    exit_qemu(QemuExitCode::Success)
}

#[test_case]
fn trivial_assertion() {
    assert_eq!(1, 1);
}
#[test_case]
fn strlen_char16_returns_valid_len() {
    assert_eq!(
        strlen_char16(CStrPtr16::from_ptr(
            // "hello" contains 5 chars
            "h\0e\0l\0l\0o\0\0\0".as_ptr().cast::<u16>()
        )),
        5
    );
}
