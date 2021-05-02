#![no_std]
#![feature(alloc_error_handler)]

use alloc::string::String;
use core::fmt;
use core::mem::size_of;
use core::panic::PanicInfo;

#[repr(packed)]
#[allow(dead_code)]
pub struct DirectoryEntry {
    inode: u64,
    next_offset: u64,
    this_size: u16,
    d_type: u8,
}

impl DirectoryEntry {
    pub fn inode(&self) -> u64 {
        self.inode
    }
    pub fn size(&self) -> usize {
        self.this_size as usize
    }
}

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
    fn sys_open(filename: *const u8, flags: u32, mode: u32) -> i32;
    fn sys_exit(code: i32) -> !;
    pub fn sys_getdents64(fd: u32, buf: *mut u8, buf_size: usize) -> i32;
}

pub fn getchar() -> u8 {
    let mut c: u8 = 0;
    unsafe {
        sys_read(0, &mut c as *mut u8, size_of::<u8>());
    }
    c
}
pub fn print_string(s: &str) {
    unsafe {
        sys_write(1, s.as_ptr(), s.len());
    }
}
pub fn putchar(c: u8) {
    unsafe {
        sys_write(1, &c as *const u8, size_of::<u8>());
    }
}
pub fn open(filename: &str, flags: u32, mode: u32) -> i32 {
    let mut filename_terminated = String::from(filename);
    filename_terminated.push('\0');
    unsafe { sys_open(filename_terminated.as_ptr(), flags, mode) }
}
pub fn exit(code: i32) -> ! {
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

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("PANIC!!!");
    println!("{}", info);
    unsafe { sys_exit(1) }
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
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

extern crate alloc;

use alloc::alloc::{GlobalAlloc, Layout};
use core::ptr::null_mut;

trait MutableAllocator {
    fn alloc(&mut self, layout: Layout) -> *mut u8;
    fn dealloc(&mut self, _ptr: *mut u8, _layout: Layout);
}

const ALLOCATOR_BUF_SIZE: usize = 0x2000;
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
        unsafe { self.buf.as_mut_ptr().add(self.used_bytes - layout.size()) }
    }
    fn dealloc(&mut self, _ptr: *mut u8, _layout: Layout) {}
}
unsafe impl GlobalAlloc for GlobalAllocatorWrapper {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        ALLOCATOR.allocator.alloc(layout)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        ALLOCATOR.allocator.dealloc(ptr, layout);
    }
}
