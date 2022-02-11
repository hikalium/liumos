use alloc::string::String;
use alloc::string::ToString;
use alloc::vec::Vec;

const SUPPORTED_PROTOCOL: &str = "http://";

#[derive(Debug)]
pub struct ParsedUrl {
    scheme: String,
    pub host: String,
    pub port: u16,
    pub path: String,
}

impl ParsedUrl {
    pub fn new(u: String) -> Self {
        let url;
        if u.starts_with(SUPPORTED_PROTOCOL) {
            url = u.split_at(SUPPORTED_PROTOCOL.len()).1.to_string();
        } else {
            url = u;
        }

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
            scheme: String::from("http"),
            host: host,
            port: port,
            path: path,
        }
    }
}
