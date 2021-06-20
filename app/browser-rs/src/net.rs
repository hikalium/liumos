use alloc::string::String;
use alloc::vec::Vec;

use crate::http::{HttpRequest, Method};
use crate::url::ParsedUrl;
use liumlib::*;

pub const AF_INET: u32 = 2;

/// For TCP.
pub const _SOCK_STREAM: u32 = 1;
/// For UDP.
pub const SOCK_DGRAM: u32 = 2;

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
    let v: Vec<&str> = host.splitn(2, ':').collect();
    let ip = if v.len() == 2 || v.len() == 1 {
        v[0]
    } else {
        panic!("invalid host name: {}", host);
    };
    ip_to_int(ip)
}

fn htons(port: u16) -> u16 {
    if cfg!(target_endian = "big") {
        port
    } else {
        port.swap_bytes()
    }
}

pub fn udp_response(parsed_url: &ParsedUrl) -> String {
    let http_request = HttpRequest::new(Method::Get, parsed_url);

    let socket_fd = match socket(AF_INET, SOCK_DGRAM, 0) {
        Some(fd) => fd,
        None => panic!("can't create a socket file descriptor"),
    };
    let mut address = SockAddr::new(
        AF_INET as u16,
        htons(parsed_url.port),
        inet_addr(&parsed_url.host),
    );
    let mut request = http_request.string();

    println!("----- sending a request -----");
    println!("{}", request);

    if sendto(&socket_fd, &mut request, 0, &address) < 0 {
        panic!("failed to send a request: {:?}", request);
    }

    let mut buf = [0; 1000];
    let length = recvfrom(&socket_fd, &mut buf, 0, &mut address);
    if length < 0 {
        panic!("failed to receive a response");
    }

    close(&socket_fd);

    match String::from_utf8(buf.to_vec()) {
        Ok(s) => s,
        Err(e) => panic!("failed to convert u8 array to string: {}", e),
    }
}
