use alloc::string::String;
use liumlib::gui::*;
use liumlib::*;

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
            Err(_) => panic!("Failed to create window"),
            Ok(w) => w,
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
            0xffffff,
            0,
            0,
            self.width as i64,
            self.height as i64,
        ) {
            Ok(()) => {}
            Err(e) => println!("{}", e),
        };
        self.buffer.flush();
    }
}
