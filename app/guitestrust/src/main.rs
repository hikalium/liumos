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

struct RectOutlineClockwiseIterator {
    x: i64,
    y: i64,
    width: i64,
    height: i64,
}

impl Iterator for RectOutlineClockwiseIterator {
    type Item = (i64, i64);
    fn next(&mut self) -> Option<Self::Item> {
        if self.y == 0 {
            self.x += 1;
            if self.x >= self.width {
                self.x = self.width - 1;
                self.y = 1;
            }
            return Some((self.x, self.y));
        }
        if self.x == self.width - 1 {
            self.y += 1;
            if self.y >= self.height {
                self.y = self.height - 1;
                self.x = self.width - 2;
            }
            return Some((self.x, self.y));
        }
        if self.y == self.height - 1 {
            self.x -= 1;
            if self.x < 0 {
                self.x = 0;
                self.y = self.height - 2;
            }
            return Some((self.x, self.y));
        }
        if self.x == 0 {
            self.y -= 1;
            if self.y < 0 {
                self.y = 0;
                self.x = 1;
            }
            return Some((self.x, self.y));
        }
        return None;
    }
}

fn draw_line_test_pattern<T: BitmapImageBuffer>(buf: &T) -> core::result::Result<(), String> {
    draw_rect(buf, 0xffffff, 0, 0, buf.width(), buf.height())?;
    let step: u32 = 1;
    let mut c: u32 = 0xc00000;
    for (px, py) in [
        (0, 0),
        (0, buf.height() - 1),
        (buf.width() - 1, 0),
        (buf.width() - 1, buf.height() - 1),
    ]
    .iter()
    {
        for (x, y) in (RectOutlineClockwiseIterator {
            x: 0,
            y: 0,
            width: buf.width(),
            height: buf.height(),
        })
        .take((buf.width() * 2 + buf.height() * 2) as usize)
        .step_by(step as usize)
        {
            draw_line(buf, c, *px, *py, x, y)?;
            c += step;
            buf.flush();
        }
    }
    for (x, y) in (RectOutlineClockwiseIterator {
        x: 0,
        y: 0,
        width: buf.width(),
        height: buf.height(),
    })
    .take((buf.width() * 2 + buf.height() * 2) as usize)
    .step_by(step as usize)
    {
        draw_line(buf, c, x, y, buf.width() - 1 - x, buf.height() - 1 - y)?;
        c += step;
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
    draw_line_test_pattern(&w).unwrap();
    draw_bouncing_rects(&w).unwrap();
    draw_test_pattern(&w);
    println!("PASS: GUI test from Rust");
}
