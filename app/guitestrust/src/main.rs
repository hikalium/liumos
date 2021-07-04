#![no_std]
#![no_main]
#![feature(rustc_private)]

use liumlib::gui::*;
use liumlib::*;

fn draw_test_pattern<T: BitmapImageBuffer>(buf: &T) {
    for i in 0..1000 {
        for y in 0..buf.height() {
            for x in 0..buf.width() {
                unsafe {
                    let p = buf
                        .buf()
                        .add((y * buf.pixels_per_line() + x) * buf.bytes_per_pixel());
                    // ARGB
                    // *p.add(3) = alpha;
                    *p.add(2) = ((x + i) * 4) as u8;
                    *p.add(1) = ((y + i) * 4) as u8;
                    *p.add(0) = 0;
                }
            }
        }
        buf.flush();
    }
}

entry_point!(main);
fn main() {
    let w = match create_window(512, 256) {
        Err(_) => panic!("Failed to create window"),
        Ok(w) => w,
    };
    draw_test_pattern(&w);
    println!("PASS: GUI test from Rust");
}
