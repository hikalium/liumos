#![no_std]
#![no_main]

use core::convert::TryInto;
use core::mem::size_of;
use core::panic::PanicInfo;

#[link(name = "liumos", kind = "static")]
extern "C" {
    fn sys_read(fp: i32, str: *mut u8, len: usize);
    fn sys_write(fp: i32, str: *const u8, len: usize);
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
fn print(s: &str) {
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

fn main() {
    let s = "Hello, Rust world on liumOS!\n";
    print(s);
    loop {
        let c = getchar();
        putchar(c);
        if c == 'q' as u8 {
            break;
        }
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    exit(1);
}
