#![no_std]
#![no_main]
#![feature(asm)]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

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
    let mut writer = Writer {};
    writeln!(writer, "[FAIL]");
    write!(writer, "{}", info);
    exit_qemu(QemuExitCode::Fail)
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    #[cfg(test)]
    test_panic(_info);

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

pub struct Writer {}

impl Writer {
    pub fn write_str(&mut self, s: &str) {
        com_send_str(IO_ADDR_COM2, s);
    }
}

impl fmt::Write for Writer {
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
pub struct EFISimpleTextOutputProtocol {
    header: EFITableHeader,
}

#[repr(C)]
pub struct EFISystemTable<'a> {
    header: EFITableHeader,
    firmware_vendor: CStrPtr16,
    firmware_revision: u32,
    console_in_handle: EFIHandle,
    con_in: &'a EFISimpleTextOutputProtocol,
}

pub struct EFIHandle {}

const EFI_SYSTEM_TABLE_SIGNATURE: u64 = 0x5453595320494249;

#[no_mangle]
pub extern "win64" fn efi_entry(_image_handle: &EFIHandle, efi_system_table: &EFISystemTable) -> ! {
    #[cfg(test)]
    test_main();

    use core::fmt::Write;
    com_initialize(IO_ADDR_COM2);
    let mut writer = Writer {};
    writeln!(writer, "liumOS loader starting...").unwrap();
    assert_eq!(
        efi_system_table.header.signature,
        EFI_SYSTEM_TABLE_SIGNATURE
    );
    writeln!(writer, "vendor: {}", efi_system_table.firmware_vendor).unwrap();
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
        let mut writer = Writer {};
        write!(writer, "{}...\t", core::any::type_name::<T>()).unwrap();
        self();
        writeln!(writer, "[PASS]").unwrap();
    }
}

#[cfg(test)]
fn test_runner(tests: &[&dyn Testable]) -> ! {
    use core::fmt::Write;
    com_initialize(IO_ADDR_COM2);
    let mut writer = Writer {};
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
