#![no_std]
#![no_main]
#![feature(alloc_error_handler)]

use alloc::vec::Vec;
use core::convert::TryInto;
use core::fmt;
use core::mem::size_of;
use core::panic::PanicInfo;
use core::str;

extern crate alloc;

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
    fn sys_open(filename: *const u8, flags: u32, mode: u32) -> u64;
    fn sys_exit(code: i32) -> !;
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("PANIC!!!");
    println!("{}", info);
    unsafe { sys_exit(1) }
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

use alloc::alloc::{GlobalAlloc, Layout};
use core::ptr::null_mut;

pub struct Dummy;

#[global_allocator]
static ALLOCATOR: Dummy = Dummy;

#[alloc_error_handler]
fn alloc_error_handler(layout: alloc::alloc::Layout) -> ! {
    panic!("allocation error: {:?}", layout)
}

unsafe impl GlobalAlloc for Dummy {
    unsafe fn alloc(&self, _layout: Layout) -> *mut u8 {
        null_mut()
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        panic!("dealloc should be never called")
    }
}

fn main() {
    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let mut buf: [u8; 32] = [0; 32];
    let mut buf_used = 0;

    let mut v: Vec<i32> = Vec::new();
    v.push(3);
    v.push(1);
    v.push(4);
    println!("{:?}", v);

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
