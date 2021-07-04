#![no_std]
#![no_main]
#![feature(rustc_private)]

extern crate alloc;

use alloc::string::String;
use liumlib::gui::*;
use liumlib::*;

fn draw_test_pattern<T: BitmapImageBuffer>(buf: &T) {
    for i in 0..500 {
        for y in 0..buf.height() {
            for x in 0..buf.width() {
                unsafe {
                    let p = buf.pixel_at(x, y);
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

fn draw_bouncing_rects<T: BitmapImageBuffer>(buf: &T) -> core::result::Result<(), String> {
    let rect_size = 32;
    let mut dx = 1;
    let mut dy = 1;
    let mut px = 0;
    let mut py = 0;
    for i in 0..2048 {
        draw_rect(buf, 0xff0000 + i * 64, px, py, rect_size, rect_size)?;
        if px + dx >= buf.width() - rect_size {
            dx = -dx;
        }
        if px + dx < 0 {
            dx = -dx;
        }
        if py + dy >= buf.height() - rect_size {
            dy = -dy;
        }
        if py + dy < 0 {
            dy = -dy;
        }
        px += dx;
        py += dy;
        buf.flush();
    }
    Ok(())
}

entry_point!(main);
fn main() {
    let w = match create_window(512, 256) {
        Err(_) => panic!("Failed to create window"),
        Ok(w) => w,
    };
    draw_bouncing_rects(&w).unwrap();
    draw_test_pattern(&w);
    println!("PASS: GUI test from Rust");
}
