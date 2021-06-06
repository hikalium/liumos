#![no_std]
#![no_main]
#![feature(rustc_private)]

extern crate alloc;
extern crate compiler_builtins;

use compiler_builtins::mem::memcpy;
use core::mem::size_of;
use core::ptr::null_mut;

#[repr(packed)]
struct BMPFileHeader {
    signature: [u8; 2],
    file_size: u32,
    reserved: u32,
    offset_to_data: u32, // offset in file to pixels
}

#[repr(packed)]
struct BMPInfoV3Header {
    info_size: u32, // = 0x38 for Size of data follows this header
    xsize: i32,
    ysize: i32,
    planes: u16,            // = 1
    bpp: u16,               // = 32
    compression_type: u32,  // = 3: bit field, {B,G,R,A}
    image_data_size: u32,   // Size of data which follows this header
    pixel_per_meter_x: u32, // = 0x2E23 = 300 ppi
    pixel_per_meter_y: u32,
    padding: u64,
    r_mask: u32,
    g_mask: u32,
    b_mask: u32,
}

use liumlib::*;

struct WindowBuffer {
    fd: i32,
    file_buf: *mut u8,
    file_size: usize,
    bmp_buf: *mut u8,
    width: usize,
    height: usize,
}

trait BitmapImageBuffer {
    fn bytes_per_pixel(&self) -> usize;
    fn pixels_per_line(&self) -> usize;
    fn width(&self) -> usize;
    fn height(&self) -> usize;
    fn buf(&self) -> *mut u8;
    fn flush(&self);
}

impl BitmapImageBuffer for WindowBuffer {
    fn bytes_per_pixel(&self) -> usize {
        4
    }
    fn pixels_per_line(&self) -> usize {
        self.width
    }
    fn width(&self) -> usize {
        self.width
    }
    fn height(&self) -> usize {
        self.height
    }
    fn buf(&self) -> *mut u8 {
        self.bmp_buf
    }
    fn flush(&self) {
        flush_window_buffer(&self);
    }
}

fn create_window_buffer(width: usize, height: usize) -> core::result::Result<WindowBuffer, ()> {
    // Open the window file
    let fd = open("window.bmp", liumlib::O_RDWR | O_CREAT, 0664);
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
        fd: fd.number(),
        file_buf: file_buf,
        file_size: file_size,
        bmp_buf: unsafe { file_buf.add(header_size_with_padding) },
        width: width,
        height: height,
    })
}

fn flush_window_buffer(w: &WindowBuffer) {
    msync(w.file_buf, w.file_size, MS_SYNC);
}

fn draw_test_pattern<T: BitmapImageBuffer>(buf: &T) {
    for y in 0..buf.height() {
        for x in 0..buf.width() {
            for i in 0..buf.bytes_per_pixel() {
                unsafe {
                    let p = buf
                        .buf()
                        .add((y * buf.pixels_per_line() + x) * buf.bytes_per_pixel());
                    // ARGB
                    // *p.add(3) = alpha;
                    *p.add(2) = (x * 4) as u8;
                    *p.add(1) = (y * 4) as u8;
                    *p.add(0) = (x * x / 32 + y * y / 32) as u8;
                }
            }
        }
    }
    buf.flush();
}

entry_point!(main);
fn main() {
    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let w = create_window_buffer(512, 256);
    if w.is_err() {
        println!("Failed to create window");
        return;
    }
    let w = w.unwrap();
    println!("Window created");
    draw_test_pattern(&w);
}
