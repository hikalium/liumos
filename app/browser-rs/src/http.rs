use alloc::string::String;
use alloc::vec::Vec;

use crate::ParsedUrl;

#[derive(Debug)]
pub enum Method {
    Get,
}

#[derive(Debug)]
struct Header {
    key: String,
    value: String,
}

#[derive(Debug)]
pub struct HTTPRequest {
    method: Method,
    path: String,
    version: String,
    headers: Vec<Header>,
    body: String,
}

impl HTTPRequest {
    pub fn new(method: Method, url: ParsedUrl) -> Self {
        Self {
            method,
            path: String::from(&url.path),
            version: String::from("HTTP/1.1"),
            headers: Vec::new(),
            body: String::from("Hello world"),
        }
    }
}
