#![no_std]
#![no_main]

use core::convert::TryInto;
use core::panic::PanicInfo;

extern crate libc;

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_write(fp: i32, str: *const u8, len: u64);
    fn sys_exit(code: i32);
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

fn print(s: &str) {
    unsafe {
        sys_write(1, s.as_ptr(), s.len().try_into().unwrap());
    }
}
fn exit(code: i32) {
    unsafe {
        sys_exit(code);
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    let s = "Hello, Rust world on liumOS!\n";
    print(s);
    exit(1);
    loop {}
}
