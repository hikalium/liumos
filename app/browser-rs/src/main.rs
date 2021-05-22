#![no_std]
#![no_main]

mod http;

extern crate alloc;

use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;
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
    port: u16,
    path: String,
}

impl ParsedUrl {
    fn new(_url: String) -> Self {
        Self {
            scheme: String::from("http"),
            host: String::from("127.0.0.1:8888"),
            //host: String::from("10.0.0.2:8888"),
            port: 8888,
            path: String::from("index.html"),
        }
    }
}

fn ip_to_int(ip: &str) -> u32 {
    let ip_blocks: Vec<&str> = ip.split('.').collect();
    if ip_blocks.len() != 4 {
        return 0;
    }

    (ip_blocks[3].parse::<u32>().unwrap() << 24)
        | (ip_blocks[2].parse::<u32>().unwrap() << 16)
        | (ip_blocks[1].parse::<u32>().unwrap())
        | (ip_blocks[0].parse::<u32>().unwrap())
}

fn inet_addr(host: &str) -> u32 {
    match host.split_once(':') {
        Some((ip, port)) => ip_to_int(ip),
        None => ip_to_int(host),
    }
}

fn htons(port: u16) -> u16 {
    if cfg!(target_endian = "big") {
        port
    } else {
        port.swap_bytes()
    }
}

entry_point!(main);
fn main() {
    let help_flag = "--help".to_string();
    let url_flag = "--url".to_string();
    let mut url = "http://127.0.0.1:8888/index.html";
    //let mut url = "http://10.0.0.2:8888/index.html";
    for arg in env::args() {
        if help_flag == *arg {
            println!("HELP!!!!!!!!!!!!!!!!!!");
            exit(0);
        }

        if url_flag == *arg {
            url = *arg;
        }
    }

    let parsed_url = ParsedUrl::new(url.to_string());
    let http_request = HTTPRequest::new(Method::Get, &parsed_url);

    let socket_fd = match socket(AF_INET, SOCK_DGRAM, 0) {
        Some(fd) => fd,
        None => panic!("can't create a socket file descriptor"),
    };
    let address = SockAddr::new(
        AF_INET as u16,
        htons(parsed_url.port),
        inet_addr(&parsed_url.host),
    );
    let mut request = http_request.string();
    sendto(&socket_fd, &mut request, 0, &address);

    close(&socket_fd);
}
