#![no_std]
#![feature(alloc_error_handler)]
#![feature(rustc_private)]

extern crate alloc;

pub mod gui;

use alloc::alloc::{GlobalAlloc, Layout};
use alloc::string::String;
use alloc::vec::Vec;
use core::fmt;
use core::mem::size_of;
use core::panic::PanicInfo;
use core::ptr::null_mut;

pub const O_RDWR: u32 = 2;
pub const O_CREAT: u32 = 1 << 6;

pub const PROT_READ: u32 = 0x01;
pub const PROT_WRITE: u32 = 0x02;

pub const MAP_FAILED: *mut u8 = u64::MAX as *mut u8;
pub const MAP_SHARED: u32 = 0x01;
pub const MAP_PRIVATE: u32 = 0x02;
pub const MAP_ANONYMOUS: u32 = 0x20;

pub const MS_SYNC: i32 = 4;

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

#[derive(Debug)]
pub struct FileDescriptor {
    fd: i32,
}

impl FileDescriptor {
    pub fn number(&self) -> i32 {
        self.fd
    }
}

impl Drop for FileDescriptor {
    fn drop(&mut self) {
        unsafe {
            sys_close(self.fd);
        }
    }
}

#[repr(C)]
#[derive(Debug)]
struct InAddr {
    /// IP address with big endian
    s_addr: u32,
}

impl InAddr {
    fn new(s_addr: u32) -> Self {
        Self { s_addr }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct SockAddr {
    sin_family: u16,
    sin_port: u16,
    in_addr: InAddr,
}

impl SockAddr {
    pub fn new(sin_family: u16, sin_port: u16, s_addr: u32) -> Self {
        Self {
            sin_family,
            sin_port,
            in_addr: InAddr::new(s_addr),
        }
    }
}

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
    fn sys_open(filename: *const u8, flags: u32, mode: u32) -> i32;
    pub fn sys_close(fp: i32) -> i32;
    fn sys_mmap(addr: *mut u8, size: usize, prot: u32, flags: u32, fd: i32, offset: u32)
        -> *mut u8;
    fn sys_munmap(addr: *mut u8, size: usize) -> i32;
    fn sys_msync(addr: *mut u8, size: usize, flags: i32) -> i32;
    fn sys_socket(domain: u32, socket_type: u32, protocol: u32) -> i32;
    fn sys_sendto(
        sockfd: u32,
        buf: *mut u8,
        len: usize,
        flags: u32,
        dest_addr: &SockAddr,
        addrlen: usize,
    ) -> i64;
    fn sys_recvfrom(
        sockfd: u32,
        buf: *mut u8,
        len: usize,
        flags: u32,
        src_addr: &mut SockAddr,
        addrlen: usize,
    ) -> i64;
    fn sys_exit(code: i32) -> !;
    fn sys_ftruncate(fd: u32, size: usize) -> i32;
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

pub fn close(fd: &FileDescriptor) -> i32 {
    unsafe { sys_close(fd.fd) }
}
pub fn mmap(
    addr: *mut u8,
    size: usize,
    prot: u32,
    flags: u32,
    fd: &FileDescriptor,
    offset: u32,
) -> *mut u8 {
    unsafe { sys_mmap(addr, size, prot, flags, fd.fd, offset) }
}

/// Returns allocated memory addr or NULL if failed.
pub fn alloc_page() -> *mut u8 {
    unsafe {
        sys_mmap(
            null_mut(),
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

pub fn msync(addr: *mut u8, size: usize, flags: i32) -> i32 {
    unsafe { sys_msync(addr, size, flags) }
}

pub fn socket(domain: u32, socket_type: u32, protocol: u32) -> Option<FileDescriptor> {
    let fd = unsafe { sys_socket(domain, socket_type, protocol) };
    if fd < 0 {
        None
    } else {
        Some(FileDescriptor { fd })
    }
}

pub fn sendto(sockfd: &FileDescriptor, buf: &mut String, flags: u32, dest_addr: &SockAddr) -> i64 {
    unsafe {
        sys_sendto(
            sockfd.fd as u32,
            buf.as_mut_ptr(),
            buf.len(),
            flags,
            dest_addr,
            0,
        )
    }
}

pub fn recvfrom(
    sockfd: &FileDescriptor,
    buf: &mut [u8],
    flags: u32,
    src_addr: &mut SockAddr,
) -> i64 {
    let len = buf.len();
    unsafe { sys_recvfrom(sockfd.fd as u32, buf.as_mut_ptr(), len, flags, src_addr, 0) }
}

pub fn ftruncate(fd: &FileDescriptor, size: usize) -> i32 {
    unsafe { sys_ftruncate(fd.fd as u32, size) }
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

trait MutableAllocator {
    fn alloc(&mut self, layout: Layout) -> *mut u8;
    fn dealloc(&mut self, _ptr: *mut u8, _layout: Layout);
}

const ALLOCATOR_BUF_SIZE: usize = 0x100000;
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
