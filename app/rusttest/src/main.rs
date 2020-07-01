#![no_std]
#![no_main]

use core::panic::PanicInfo;

extern crate libc;

#[link(name = "hello", kind = "static")]
extern "C" {
    fn HelloFromC();
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    unsafe {
        HelloFromC();
    }
    loop {}
}
