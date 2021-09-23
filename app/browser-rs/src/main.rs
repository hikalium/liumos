#![no_std]
#![no_main]

mod gui;
mod http;
mod net;
mod renderer;
mod url;

extern crate alloc;

use alloc::string::ToString;
use liumlib::gui::*;
use liumlib::*;

use crate::gui::ApplicationWindow;
use crate::net::udp_response;
use crate::renderer::render;
use crate::url::ParsedUrl;

fn help_message() {
    println!("Usage: browser-rs.bin [ OPTIONS ]");
    println!("       -u, --url      URL. Default: http://127.0.0.1:8888/index.html");
    println!("       -h, --help     Show this help message.");
    exit(0);
}

entry_point!(main);
fn main() {
    let mut url = "http://127.0.0.1:8888/index.html";

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
    }

    let parsed_url = ParsedUrl::new(url.to_string());

    let _app = ApplicationWindow::new(512, 256, "my browser".to_string()).initialize();

    let response = udp_response(&parsed_url);

    println!("----- receiving a response -----");
    println!("{}", response);

    render(response);
}
