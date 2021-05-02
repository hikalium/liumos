#![no_std]
#![no_main]

extern crate alloc;

use alloc::string::String;
use liumlib::*;

const SHELIUM_VERSION: &str = env!("CARGO_PKG_VERSION");
const SHELIUM_GIT_HASH: &str = env!("GIT_HASH");

fn main() {
    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let mut line = String::with_capacity(32);

    loop {
        let c = getchar();
        putchar(c);
        if c == b'\n' {
            println!("{:?}", line);
            if line == "q" {
                break;
            } else if line == "version" {
                println!("shelium {} {}", SHELIUM_VERSION, SHELIUM_GIT_HASH);
            }
            line.truncate(0);
            continue;
        }
        line.push(c as char);
    }
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    exit(1);
}
