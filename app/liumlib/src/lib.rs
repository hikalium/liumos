#![no_std]
#![feature(alloc_error_handler)]

use alloc::string::String;
use alloc::vec::Vec;
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

pub struct FileDescriptor {
    fd: i32,
}

impl Drop for FileDescriptor {
    fn drop(&mut self) {
        unsafe {
            sys_close(self.fd);
        }
    }
}

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
    fn sys_open(filename: *const u8, flags: u32, mode: u32) -> i32;
    fn sys_close(fp: i32) -> i32;
    fn sys_mmap(addr: *mut u8, size: usize, prot: u32, flags: u32, fd: i32, offset: u32)
        -> *mut u8;
    fn sys_munmap(addr: *mut u8, size: usize) -> i32;
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
pub fn open(filename: &str, flags: u32, mode: u32) -> Option<FileDescriptor> {
    let mut filename_terminated = String::from(filename);
    filename_terminated.push('\0');
    let fd = unsafe { sys_open(filename_terminated.as_ptr(), flags, mode) };
    if fd < 0 {
        None
    } else {
        Some(FileDescriptor { fd })
    }
}
pub fn close(fd: i32) -> i32 {
    unsafe { sys_close(fd) }
}
const PROT_READ: u32 = 0x01;
const PROT_WRITE: u32 = 0x02;
const MAP_PRIVATE: u32 = 0x02;
const MAP_ANONYMOUS: u32 = 0x20;
/// Returns allocated memory addr or NULL if failed.
pub fn alloc_page() -> *mut u8 {
    unsafe {
        sys_mmap(
            core::ptr::null_mut::<u8>(),
            4096,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0,
        )
    }
}
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub fn free_page(addr: *mut u8) -> i32 {
    unsafe { sys_munmap(addr, 4096) }
}
pub fn exit(code: i32) -> ! {
    unsafe {
        sys_exit(code);
    }
}
pub fn getdents64(fd: &FileDescriptor, buf: &mut [u8]) -> i32 {
    unsafe { sys_getdents64(fd.fd as u32, buf.as_mut_ptr(), buf.len()) }
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

unsafe fn strlen(s: *const u8) -> usize {
    let mut count = 0;
    loop {
        if *s.add(count) == 0 {
            break;
        }
        count += 1;
    }
    count
}

static mut ARGS: Vec<&str> = Vec::new();

pub mod env {
    use alloc::vec::Vec;
    pub fn args() -> &'static Vec<&'static str> {
        unsafe { &super::ARGS }
    }
}

/// # Safety
///
/// This function should be called only from entry_point function
pub unsafe fn setup_liumlib(argc: usize, argv: *const *const u8) {
    for i in 0..argc {
        let p = argv.add(i);
        let s = core::slice::from_raw_parts(*p, strlen(*p));
        let s = core::str::from_utf8(s);
        match s {
            Ok(s) => ARGS.push(&s),
            Err(e) => panic!("Failed to convert argv to utf8 str: {}", e),
        }
    }
}

#[macro_export]
macro_rules! entry_point {
    // c.f. https://docs.rs/bootloader/0.6.4/bootloader/macro.entry_point.html
    ($path:path) => {
        #[no_mangle]
        pub unsafe extern "C" fn _start_rs(argc: usize, argv: *const *const u8) -> ! {
            // validate the signature of the program entry point
            let f: fn() -> () = $path;
            unsafe {
                setup_liumlib(argc, argv);
            }
            f();
            exit(0);
        }
    };
}
