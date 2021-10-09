#![no_std]
#![no_main]
#![feature(exclusive_range_pattern)]

mod gui;
mod http;
mod net;
mod renderer;
mod url;

extern crate alloc;

use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;
use liumlib::gui::draw_rect;
use liumlib::gui::BitmapImageBuffer;
use liumlib::*;

use crate::gui::ApplicationWindow;
use crate::net::udp_response;
use crate::renderer::css::Token;
use crate::renderer::render;
use crate::url::ParsedUrl;

fn help_message() {
    println!("Usage: browser-rs.bin [ OPTIONS ]");
    println!("       -u, --url      URL. Default: http://127.0.0.1:8888/index.html");
    println!("       -h, --help     Show this help message.");
    println!("       -d, --default  Show a default page with embedded HTML content for test.");
    exit(0);
}

entry_point!(main);
fn main() {
    let mut url = "http://127.0.0.1:8888/index.html";
    let mut default = false;

    let args = env::args();
    for i in 1..args.len() {
        if "--help".to_string() == args[i] || "-h" == args[i] {
            help_message();
            return;
        }

        if "--url".to_string() == args[i] || "-u".to_string() == args[i] {
            if i + 1 >= args.len() {
                help_message();
            }
            url = args[i + 1];
        }

        if "--default".to_string() == args[i] || "-d".to_string() == args[i] {
            default = true;
        }
    }

    let mut app = ApplicationWindow::new(512, 256, "my browser".to_string());
    app.initialize();

    if default {
        let default_page =
            "<html><head><style>h1{background-color:blue;}</style></head><body></body></html>";

        let rules = render(default_page.to_string(), &app);
        if rules[0].key == Token::Ident("background-color".to_string()) {
            let color = match &rules[0].value {
                Token::Ident(color_string) => match color_string.as_str() {
                    "red" => 0xff0000,
                    "green" => 0x00ff00,
                    "blue" => 0x0000ff,
                    _ => unimplemented!("unsupported"),
                },
                _ => unimplemented!("unsupported"),
            };

            draw_rect(&app.buffer, color, 10, 10, 210, 210).expect("update a window");
            app.buffer.flush();
        }
    } else {
        let parsed_url = ParsedUrl::new(url.to_string());
        println!("parsed_url: {:?}", parsed_url);

        println!("----- receiving a response -----");
        let response = udp_response(&parsed_url);
        println!("{}", response);

        render(response, &app);
    }
}
