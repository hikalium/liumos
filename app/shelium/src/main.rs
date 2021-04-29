#![no_std]
#![no_main]

extern crate alloc;

use alloc::vec::Vec;
use core::str;
use liumlib::*;

fn main() {
    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let mut buf: [u8; 32] = [0; 32];
    let mut buf_used = 0;

    let mut v: Vec<i32> = Vec::new();
    for i in 0..10 {
        v.push(i);
    }
    println!("{:?}", v);

    loop {
        let c = getchar();
        putchar(c);
        if c == b'q' {
            break;
        }
        if buf_used < buf.len() - 1 {
            buf[buf_used] = c;
            buf_used += 1;
            buf[buf_used] = 0;
        }
        if c == b'\n' {
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
