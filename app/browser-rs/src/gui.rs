use alloc::string::String;
use liumlib::gui::*;

pub struct ApplicationWindow {
    width: u64,
    height: u64,
    title: String,
    pub content_x: i64,
    pub content_y: i64,
    pub content_w: i64,
    pub content_h: i64,
    pub buffer: WindowBuffer,
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
            content_x: 0,
            content_y: 0,
            content_w: 0,
            content_h: 0,
            buffer: window_buffer,
        }
    }

    pub fn initialize(&mut self) {
        let component_margin = 2;
        let window_margin = 4;

        let button_width = 18;
        let address_bar_height = 20;

        self.content_x = window_margin;
        self.content_y = address_bar_height * 2 + window_margin + component_margin * 2;
        self.content_w = self.width as i64 - window_margin * 2;
        self.content_h = self.height as i64
            - (address_bar_height * 2 + window_margin * 2 + component_margin * 2);

        // base window
        match draw_rect(
            &self.buffer,
            0xb8b8b8, // grey
            0,
            0,
            self.width as i64,
            self.height as i64,
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a background window: {:?}", e),
        };

        // title bar
        match draw_rect(
            &self.buffer,
            0x000078,                              // blue
            window_margin,                         // px
            window_margin,                         // py
            self.width as i64 - window_margin * 2, // w
            address_bar_height,                    // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a bar: {:?}", e),
        };

        // close button
        match draw_rect(
            &self.buffer,
            0xb8b8b8,                                                              // grey
            self.width as i64 - (button_width + component_margin + window_margin), // px
            window_margin + component_margin,                                      // py
            button_width - component_margin,                                       // w
            button_width - component_margin,                                       // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a close button: {:?}", e),
        };

        /*
        // close button (\)
        match draw_line(
            &self.buffer,
            0x000000,                                                                // black
            self.width as i64 - window_margin - component_margin - button_width + 2, // px
            window_margin + component_margin + 2,                                    // py
            self.width as i64 - window_margin - component_margin - 5,                // x1
            window_margin + component_margin + button_width - 3,                     // y1
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a line: {:?}", e),
        };

        // close button (/)
        match draw_line(
            &self.buffer,
            0x000000,                                                                // black
            self.width as i64 - window_margin - component_margin - 5,                // px
            window_margin + component_margin + 1,                                    // py
            self.width as i64 - window_margin - component_margin - button_width + 2, // x1
            window_margin + component_margin + button_width - 4,                     // y1
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a line: {:?}", e),
        };
        */

        // address bar
        match draw_rect(
            &self.buffer,
            0xffffff,                                              // white
            60 + window_margin,                                    // px
            window_margin + address_bar_height + component_margin, // py
            self.width as i64 - (60 + window_margin * 2),          // w
            address_bar_height,                                    // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a bar: {:?}", e),
        };

        // content for html
        match draw_rect(
            &self.buffer,
            0xffffff,       // white
            self.content_x, // px
            self.content_y, // py
            self.content_w, // w
            self.content_h, // h
        ) {
            Ok(()) => {}
            Err(e) => panic!("Failed to draw a background window: {:?}", e),
        };

        self.buffer.flush();
    }
}
