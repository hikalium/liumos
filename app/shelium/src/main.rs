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

trait MutableAllocator {
    fn alloc(&mut self, layout: Layout) -> *mut u8;
    fn dealloc(&mut self, _ptr: *mut u8, _layout: Layout);
}

const ALLOCATOR_BUF_SIZE: usize = 0x100;
pub struct WaterMarkAllocator {
    buf: [u8; ALLOCATOR_BUF_SIZE],
    used_bytes: usize,
}

pub struct GlobalAllocatorWrapper {
    allocator: WaterMarkAllocator,
}

#[global_allocator]
static mut ALLOCATOR: GlobalAllocatorWrapper = GlobalAllocatorWrapper {
    allocator: WaterMarkAllocator {
        buf: [0; ALLOCATOR_BUF_SIZE],
        used_bytes: 0,
    },
};

#[alloc_error_handler]
fn alloc_error_handler(layout: alloc::alloc::Layout) -> ! {
    panic!("allocation error: {:?}", layout)
}

impl MutableAllocator for WaterMarkAllocator {
    fn alloc(&mut self, layout: Layout) -> *mut u8 {
        if self.used_bytes > ALLOCATOR_BUF_SIZE {
            return null_mut();
        }
        self.used_bytes = (self.used_bytes + layout.align() - 1) / layout.align() * layout.align();
        self.used_bytes += layout.size();
        if self.used_bytes > ALLOCATOR_BUF_SIZE {
            return null_mut();
        }
        println!("alloc: Allocated {:?}, used: {}", layout, self.used_bytes);
        unsafe {
            return self.buf.as_mut_ptr().add(self.used_bytes - layout.size());
        }
    }
    fn dealloc(&mut self, _ptr: *mut u8, layout: Layout) {
        println!("dealloc: Freed {:?}", layout);
    }
}
unsafe impl GlobalAlloc for GlobalAllocatorWrapper {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        return ALLOCATOR.allocator.alloc(layout);
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        ALLOCATOR.allocator.dealloc(ptr, layout);
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
