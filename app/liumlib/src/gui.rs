extern crate alloc;
extern crate compiler_builtins;

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

fn flush_window_buffer(w: &WindowBuffer) {
    msync(w.file_buf, w.file_size, MS_SYNC);
}
