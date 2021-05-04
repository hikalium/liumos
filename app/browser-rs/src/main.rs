#![no_std]
#![no_main]

mod http;

extern crate alloc;

use alloc::string::String;
use alloc::string::ToString;
use liumlib::*;

use crate::http::{HTTPRequest, Method};

const AF_INET: u32 = 2;

/// For TCP.
const SOCK_STREAM: u32 = 1;
/// For UDP.
const SOCK_DGRAM: u32 = 2;

#[derive(Debug)]
pub struct ParsedUrl {
    scheme: String,
    host: String,
    port: u32,
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
    println!("parsed_url {:?}", &parsed_url);
    let http_request = HTTPRequest::new(Method::Get, &parsed_url);
    println!("http_request {:?}", http_request);

    let socket_fd = match socket(AF_INET, SOCK_DGRAM, 0) {
        Some(fd) => fd,
        None => panic!("can't create a socket file descriptor"),
    };
    let address = SockAddr::new(AF_INET as u16, 0 /*tmp addr*/, parsed_url.port);
    let mut request = http_request.string();
    sendto(&socket_fd, &mut request, 0, &address);

    close(&socket_fd);
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    main();
    exit(1);
}
