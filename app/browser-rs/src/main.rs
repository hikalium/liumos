#![no_std]
#![no_main]

mod http;

extern crate alloc;

use alloc::string::String;
use alloc::string::ToString;
use liumlib::*;

use crate::http::{HTTPRequest, Method};

#[derive(Debug)]
pub struct ParsedUrl {
    scheme: String,
    host: String,
    port: u16,
    path: String,
}

impl ParsedUrl {
    fn new(_url: String) -> Self {
        Self {
            scheme: String::from("http"),
            host: String::from("127.0.0.1:8888"),
            port: 8888,
            path: String::from("index.html"),
        }
    }
}

fn main() {
    //let args: Vec<String> = env::args().collect();
    //println!("args {}", args);
    println!("hello");
    let url = "http://127.0.0.1:8888/index.html";
    let parsed_url = ParsedUrl::new(url.to_string());
    println!("parsed_url {:?}", parsed_url);
    let http_request = HTTPRequest::new(Method::Get, parsed_url);
    println!("http_request {:?}", http_request);
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    exit(1);
}
