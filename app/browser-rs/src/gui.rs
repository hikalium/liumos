use alloc::string::String;
use liumlib::gui::*;

#[derive(Debug, Clone)]
pub struct ApplicationWindow {
    width: u64,
    height: u64,
    title: String,
    buffer: WindowBuffer,
}

impl ApplicationWindow {
    pub fn new(w: u64, h: u64, title: String) -> Self {
        let window_buffer = match create_window(w as usize, h as usize) {
            Ok(w) => w,
            Err(e) => panic!("Failed to create an application window: {:?}", e),
        };

        Self {
            width: w,
            height: h,
            title,
            buffer: window_buffer,
        }
    }

    pub fn initialize(&mut self) {
        match draw_rect(
            &self.buffer,
            0xffffff, // color (red, green, blue)
            0,
            0,
            self.width as i64,
            self.height as i64,
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a background window: {:?}", e),
        };

        // address bar
        match draw_rect(
            &self.buffer,
            0x2222ff,              // color (red, green, blue)
            1,                     // px
            1,                     // py
            self.width as i64 - 2, // w
            20,                    // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a bar: {:?}", e),
        };

        // close button
        match draw_rect(
            &self.buffer,
            0xff3333,               // color (red, green, blue)
            self.width as i64 - 20, // px
            3,                      // py
            16,                     // w
            16,                     // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a close button: {:?}", e),
        };

        match draw_line(
            &self.buffer,
            0xffffff,               // color
            self.width as i64 - 20, // x0
            3,                      // y0
            self.width as i64 - 5,  // x1
            18,                     // y1
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a line: {:?}", e),
        };

        match draw_line(
            &self.buffer,
            0xffffff,               // color
            self.width as i64 - 5,  // x0
            3,                      // y0
            self.width as i64 - 20, // x1
            18,                     // y1
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a line: {:?}", e),
        };

        self.buffer.flush();
    }
}
