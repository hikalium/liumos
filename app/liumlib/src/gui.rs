extern crate alloc;
extern crate compiler_builtins;

use crate::alloc::string::ToString;
use alloc::format;
use compiler_builtins::mem::memcpy;
use core::mem::size_of;
use core::ptr::null_mut;

use crate::*;

#[repr(packed)]
pub struct BMPFileHeader {
    pub signature: [u8; 2],
    pub file_size: u32,
    pub reserved: u32,
    pub offset_to_data: u32, // offset in file to pixels
}

#[repr(packed)]
pub struct BMPInfoV3Header {
    pub info_size: u32, // = 0x38 for Size of data follows this header
    pub xsize: i32,
    pub ysize: i32,
    pub planes: u16,            // = 1
    pub bpp: u16,               // = 32
    pub compression_type: u32,  // = 3: bit field, {B,G,R,A}
    pub image_data_size: u32,   // Size of data which follows this header
    pub pixel_per_meter_x: u32, // = 0x2E23 = 300 ppi
    pub pixel_per_meter_y: u32,
    pub padding: u64,
    pub r_mask: u32,
    pub g_mask: u32,
    pub b_mask: u32,
}

pub struct WindowBuffer {
    file_buf: *mut u8,
    file_size: usize,
    bmp_buf: *mut u8,
    width: usize,
    height: usize,
}

pub trait BitmapImageBuffer {
    fn bytes_per_pixel(&self) -> i64;
    fn pixels_per_line(&self) -> i64;
    fn width(&self) -> i64;
    fn height(&self) -> i64;
    fn buf(&self) -> *mut u8;
    unsafe fn pixel_at(&self, x: i64, y: i64) -> *mut u8;
    fn flush(&self);
    fn is_in_x_range(&self, py: i64) -> bool;
    fn is_in_y_range(&self, py: i64) -> bool;
}

impl BitmapImageBuffer for WindowBuffer {
    fn bytes_per_pixel(&self) -> i64 {
        4
    }
    fn pixels_per_line(&self) -> i64 {
        self.width as i64
    }
    fn width(&self) -> i64 {
        self.width as i64
    }
    fn height(&self) -> i64 {
        self.height as i64
    }
    fn buf(&self) -> *mut u8 {
        self.bmp_buf
    }
    unsafe fn pixel_at(&self, x: i64, y: i64) -> *mut u8 {
        self.buf()
            .add(((y * self.pixels_per_line() + x) * self.bytes_per_pixel()) as usize)
    }
    fn flush(&self) {
        flush_window_buffer(&self);
    }
    fn is_in_x_range(&self, px: i64) -> bool {
        0 <= px && px < self.width as i64
    }
    fn is_in_y_range(&self, py: i64) -> bool {
        0 <= py && py < self.height as i64
    }
}

pub fn create_window(width: usize, height: usize) -> core::result::Result<WindowBuffer, ()> {
    // Open the window file
    let fd = open("window.bmp", O_RDWR | O_CREAT, 0o664);
    if fd.is_none() {
        return Err(());
    }
    let fd = fd.unwrap();

    // resize the file to store the bmp data
    let header_size_with_padding =
        (size_of::<BMPFileHeader>() + size_of::<BMPInfoV3Header>() + 0xF) & !0xF; /* header size aligned to 16-byte boundary */
    let file_size = header_size_with_padding + width * height * 4;

    ftruncate(&fd, file_size);
    let file_buf = mmap(null_mut(), file_size, PROT_WRITE, MAP_SHARED, &fd, 0);

    if file_buf == MAP_FAILED {
        return Err(());
    }

    let file_header = BMPFileHeader {
        signature: [b'B', b'M'],
        file_size: file_size as u32,
        offset_to_data: header_size_with_padding as u32,
        reserved: 0,
    };

    let info_header: BMPInfoV3Header = BMPInfoV3Header {
        info_size: size_of::<BMPInfoV3Header>() as u32,
        xsize: width as i32,
        ysize: -(height as i32),
        planes: 1,
        bpp: 32,
        compression_type: 3,
        image_data_size: file_header.file_size
            - size_of::<BMPFileHeader>() as u32
            - size_of::<BMPInfoV3Header>() as u32,
        pixel_per_meter_y: 0x2E23, /* 300ppi */
        pixel_per_meter_x: 0x2E23, /* 300ppi */
        padding: 0,
        r_mask: 0xFF0000,
        g_mask: 0x00FF00,
        b_mask: 0x0000FF,
    };

    unsafe {
        memcpy(
            file_buf,
            &file_header as *const BMPFileHeader as *const u8,
            size_of::<BMPFileHeader>(),
        );
        memcpy(
            file_buf.add(size_of::<BMPFileHeader>()),
            &info_header as *const BMPInfoV3Header as *const u8,
            size_of::<BMPInfoV3Header>(),
        );
    }

    Ok(WindowBuffer {
        file_buf: file_buf,
        file_size: file_size,
        bmp_buf: unsafe { file_buf.add(header_size_with_padding) },
        width: width,
        height: height,
    })
}

pub fn draw_rect<T: BitmapImageBuffer>(
    buf: &T,
    color: u32,
    px: i64,
    py: i64,
    w: i64,
    h: i64,
) -> core::result::Result<(), String> {
    // Returns Err if the rect is not in the window area.
    if !buf.is_in_x_range(px)
        || !buf.is_in_x_range(px + w - 1)
        || !buf.is_in_y_range(py)
        || !buf.is_in_y_range(py + h - 1)
    {
        return Err("Out of range (rect)".to_string());
    }
    let r: u8 = (color >> 16) as u8;
    let g: u8 = (color >> 8) as u8;
    let b: u8 = color as u8;
    for y in py..py + h {
        for x in px..px + w {
            unsafe {
                let p = buf.pixel_at(x, y);
                // ARGB
                // *p.add(3) = alpha;
                *p.add(2) = r;
                *p.add(1) = g;
                *p.add(0) = b;
            }
        }
    }
    Ok(())
}

pub fn draw_point<T: BitmapImageBuffer>(
    buf: &T,
    color: u32,
    x: i64,
    y: i64,
) -> core::result::Result<(), String> {
    if !buf.is_in_x_range(x) || !buf.is_in_x_range(x) {
        return Err("Out of range (point)".to_string());
    }
    let r: u8 = (color >> 16) as u8;
    let g: u8 = (color >> 8) as u8;
    let b: u8 = color as u8;
    unsafe {
        let p = buf.pixel_at(x, y);
        // ARGB
        // *p.add(3) = alpha;
        *p.add(2) = r;
        *p.add(1) = g;
        *p.add(0) = b;
    }
    Ok(())
}

pub fn draw_line<T: BitmapImageBuffer>(
    buf: &T,
    color: u32,
    x0: i64,
    y0: i64,
    x1: i64,
    y1: i64,
) -> core::result::Result<(), String> {
    if !buf.is_in_x_range(x0)
        || !buf.is_in_x_range(x1)
        || !buf.is_in_y_range(y0)
        || !buf.is_in_y_range(y1)
    {
        return Err(
            format!("Out of range (line ({}, {}) => ({}, {}))", x0, y0, x1, y1).to_string(),
        );
    }

    if x1 < x0 {
        return draw_line(buf, color, x1, y1, x0, y0);
    }
    if x1 == x0 {
        if y0 <= y1 {
            for i in y0..=y1 {
                draw_point(buf, color, x0, i)?;
            }
        } else {
            for i in y1..=y0 {
                draw_point(buf, color, x0, i)?;
            }
        }
        return Ok(());
    }
    assert!(x0 < x1);
    let lx = x1 - x0 + 1;
    const MULTIPLIER: i64 = 1024 * 1024;
    let a = (y1 - y0) * MULTIPLIER / lx;
    for i in 0..lx {
        draw_line(
            buf,
            color,
            x0 + i,
            y0 + (a * i / MULTIPLIER),
            x0 + i,
            y0 + (a * (i + 1) / MULTIPLIER),
        )?;
    }
    draw_point(buf, color, x0, y0)?;
    draw_point(buf, color, x1, y1)?;
    Ok(())
}

fn flush_window_buffer(w: &WindowBuffer) {
    msync(w.file_buf, w.file_size, MS_SYNC);
}
