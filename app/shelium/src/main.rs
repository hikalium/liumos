#![no_std]
#![no_main]

use core::convert::TryInto;
use core::fmt;
use core::mem::size_of;
use core::panic::PanicInfo;
use core::str;

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
    fn sys_open(filename: *const u8, flags: u32, mode: u32) -> u64;
    fn sys_exit(code: i32) -> !;
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

fn getchar() -> u8 {
    let mut c: u8 = 0;
    unsafe {
        sys_read(0, &mut c as *mut u8, size_of::<u8>());
    }
    c
}
fn print_string(s: &str) {
    unsafe {
        sys_write(1, s.as_ptr(), s.len().try_into().unwrap());
    }
}
fn putchar(c: u8) {
    unsafe {
        sys_write(1, &c as *const u8, size_of::<u8>());
    }
}
fn exit(code: i32) -> ! {
    unsafe {
        sys_exit(code);
    }
}

pub struct StdIoWriter {}
impl StdIoWriter {}
impl fmt::Write for StdIoWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        print_string(s);
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
        ($($arg:tt)*) => (_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
        () => ($crate::print!("\n"));
            ($($arg:tt)*) => (print!("{}\n", format_args!($($arg)*)));
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    let mut writer = crate::StdIoWriter {};
    fmt::write(&mut writer, args).unwrap();
}

fn main() {
    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let mut buf: [u8; 32] = [0; 32];
    let mut buf_used = 0;
    loop {
        let c = getchar();
        putchar(c);
        if c == 'q' as u8 {
            break;
        }
        if buf_used < buf.len() - 1 {
            buf[buf_used] = c;
            buf_used += 1;
            buf[buf_used] = 0;
        }
        if c == '\n' as u8 {
            let s = str::from_utf8(&buf).unwrap();
            println!("{:?}", s);
            buf_used = 0;
        }
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    exit(1);
}
