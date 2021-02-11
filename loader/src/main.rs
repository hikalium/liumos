#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]
#![allow(clippy::zero_ptr)]
use core::convert::TryInto;
use core::fmt;
use core::panic::PanicInfo;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct CStrPtr16 {
    ptr: *const u16,
}
impl CStrPtr16 {
    #[cfg(test)]
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

const EFI_SYSTEM_TABLE_SIGNATURE: u64 = 0x5453595320494249;
const EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x9042a9de,
    data1: 0x23dc,
    data2: 0x4a38,
    data3: [0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a],
};

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

#[repr(C)]
pub struct EFIBootServicesTable {
    header: EFITableHeader,
    padding0: [u64; 4],
    get_memory_map: EFIHandle,
    padding1: [u64; 11],
    handle_protocol: EFIHandle,
    padding2: [u64; 9],
    exit_boot_services: EFIHandle,
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
pub extern "win64" fn efi_entry(
    _image_handle: *const EFIHandle,
    efi_system_table: &EFISystemTable,
) -> ! {
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
        writeln!(efi_writer, "{:?}", *graphics_output_protocol).unwrap();
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

    loop {
        hlt();
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
