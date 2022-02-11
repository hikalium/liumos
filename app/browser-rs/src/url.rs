//! RFC 1738 - Uniform Resource Locators (URL): https://datatracker.ietf.org/doc/html/rfc1738
//! RFC 3986 - Uniform Resource Identifier (URI): https://datatracker.ietf.org/doc/html/rfc3986

use alloc::string::String;
use alloc::vec::Vec;

#[derive(Debug)]
enum Protocol {
    Http,
}

impl Protocol {
    fn to_string(&self) -> String {
        match self {
            Protocol::Http => String::from("http"),
        }
    }
}

#[derive(Debug)]
pub struct ParsedUrl {
    scheme: Protocol,
    pub host: String,
    pub port: u16,
    pub path: String,
}

impl ParsedUrl {
    fn extract_scheme(url: &String) -> Protocol {
        let splitted_url: Vec<&str> = url.split("://").collect();
        if splitted_url.len() == 2 && splitted_url[0] == Protocol::Http.to_string() {
            Protocol::Http
        } else if splitted_url.len() == 1 {
            // No scheme. Set "HTTP" as a default behavior.
            Protocol::Http
        } else {
            panic!("unsupported scheme: {}", url);
        }
    }

    fn remove_scheme(url: &String, scheme: Protocol) -> String {
        // Remove "scheme://" from url if any.
        url.replacen(&(scheme.to_string() + "://"), "", 1)
    }

    pub fn new(original_url: String) -> Self {
        let scheme = Self::extract_scheme(&original_url);
        let url = Self::remove_scheme(&original_url, scheme);

        let mut host = String::new();
        let mut path = String::new();
        {
            let v: Vec<&str> = url.splitn(2, '/').collect();
            if v.len() == 2 {
                host.push_str(v[0]);
                path.push_str("/");
                path.push_str(v[1]);
            } else if v.len() == 1 {
                host.push_str(v[0]);
                path.push_str("/index.html");
            } else {
                panic!("invalid url {}", url);
            }
        }

        let port;
        {
            let v: Vec<&str> = host.splitn(2, ':').collect();
            if v.len() == 2 {
                port = v[1].parse::<u16>().unwrap();
            } else if v.len() == 1 {
                port = 8888;
            } else {
                panic!("invalid host in url {}", host);
            }
        }

        Self {
            scheme: Protocol::Http,
            host,
            port,
            path,
        }
    }
}
