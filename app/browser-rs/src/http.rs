use alloc::string::String;
use alloc::vec::Vec;

use crate::ParsedUrl;

#[derive(Debug)]
pub enum Method {
    Get,
}

impl Method {
    fn name(&self) -> String {
        match self {
            Method::Get => String::from("GET"),
        }
    }
}

#[derive(Debug)]
struct Header {
    key: String,
    value: String,
}

impl Header {
    fn new(key: String, value: String) -> Self {
        Self { key, value }
    }
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
    pub fn new(method: Method, url: &ParsedUrl) -> Self {
        let mut req = Self {
            method,
            path: String::from(&url.path),
            version: String::from("HTTP/1.1"),
            headers: Vec::new(),
            body: String::from("sending a request"),
        };

        req.add_header(String::from("Host"), String::from(&url.host));

        req
    }

    pub fn add_header(&mut self, key: String, value: String) {
        self.headers.push(Header::new(key, value));
    }

    pub fn string(&self) -> String {
        // request line
        let mut request = self.method.name();
        request.push(' ');
        request.push_str(&self.path);
        request.push(' ');
        request.push_str(&self.version);
        request.push('\n');

        // headers
        for h in &self.headers {
            request.push_str(&h.key);
            request.push_str(": ");
            request.push_str(&h.value);
            request.push('\n');
        }
        request.push('\n');

        // body
        request.push_str(&self.body);

        request
    }
}
