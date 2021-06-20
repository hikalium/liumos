#![no_std]
#![no_main]

mod http;
mod net;
mod rendering;
mod url;

extern crate alloc;

use alloc::string::ToString;
use liumlib::*;

use crate::net::udp_response;
use crate::rendering::render;
use crate::url::ParsedUrl;

fn help_message() {
    println!("Usage: browser-rs.bin [ OPTIONS ]");
    println!("       -u, --url      URL. Default: http://127.0.0.1:8888/index.html");
    exit(0);
}

entry_point!(main);
fn main() {
    let mut url = "http://127.0.0.1:8888/index.html";

    let help_flag = "--help".to_string();
    let url_flag = "--url".to_string();

    let args = env::args();
    for i in 1..args.len() {
        if help_flag == args[i] {
            help_message();
        }

        if url_flag == args[i] {
            if i + 1 >= args.len() {
                help_message();
            }
            url = args[i + 1];
        }
    }

    let parsed_url = ParsedUrl::new(url.to_string());

    let response = udp_response(&parsed_url);

    println!("----- receiving a response -----");
    println!("{}", response);

    render(response);
}
