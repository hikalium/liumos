//! RFC 1738 - Uniform Resource Locators (URL): https://datatracker.ietf.org/doc/html/rfc1738
//! RFC 3986 - Uniform Resource Identifier (URI): https://datatracker.ietf.org/doc/html/rfc3986

use alloc::string::String;
use alloc::string::ToString;
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

    fn extract_host(url: &String) -> String {
        let splitted_url: Vec<&str> = url.splitn(2, '/').collect();
        splitted_url[0].to_string()
    }

    fn extract_path(url: &String) -> String {
        let splitted_url: Vec<&str> = url.splitn(2, '/').collect();
        if splitted_url.len() == 2 {
            splitted_url[1].to_string()
        } else {
            // There is no path in URL so set an empty string as a default value.
            String::from("")
        }
    }

    fn extract_port(host: &String) -> u16 {
        let splitted_host: Vec<&str> = host.splitn(2, ':').collect();
        if splitted_host.len() == 2 {
            splitted_host[1].parse::<u16>().unwrap()
        } else {
            // There is no port number in URL so set 8888 as a default value.
            80
        }
    }

    pub fn new(original_url: String) -> Self {
        let scheme = Self::extract_scheme(&original_url);
        let url = Self::remove_scheme(&original_url, scheme);

        let host = Self::extract_host(&url);
        let path = Self::extract_path(&url);

        let port = Self::extract_port(&host);

        Self {
            scheme: Protocol::Http,
            host,
            port,
            path,
        }
    }
}
