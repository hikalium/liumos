#![no_std]
#![no_main]
#![feature(exclusive_range_pattern)]

mod gui;
mod http;
mod net;
mod renderer;
mod url;

extern crate alloc;

use alloc::string::ToString;
use liumlib::*;

use crate::gui::ApplicationWindow;
use crate::net::udp_response;
use crate::renderer::render;
use crate::url::ParsedUrl;

fn help_message() {
    println!("Usage: browser-rs.bin [ OPTIONS ]");
    println!("       -u, --url      URL. Default: http://127.0.0.1:8888/index.html");
    println!("       -h, --help     Show this help message.");
    println!("       -d, --default  Show a default page with embedded HTML content for test.");
    exit(0);
}

entry_point!(main);
fn main() {
    let mut url = "http://127.0.0.1:8888/index.html";
    let mut default = false;

    let args = env::args();
    for i in 1..args.len() {
        if "--help".to_string() == args[i] || "-h" == args[i] {
            help_message();
            return;
        }

        if "--url".to_string() == args[i] || "-u".to_string() == args[i] {
            if i + 1 >= args.len() {
                help_message();
            }
            url = args[i + 1];
        }

        if "--default".to_string() == args[i] || "-d".to_string() == args[i] {
            default = true;
        }
    }

    let mut app = ApplicationWindow::new(512, 256, "my browser".to_string());
    app.initialize();

    if default {
        let default_page = r#"<html>
            <head>
              <style>
                .leaf {
                  background-color: green;
                  height: 5;
                  width: 5;
                }
                #leaf1 {
                  margin-top: 50;
                  margin-left: 275;
                }
                #leaf2 {
                  margin-left: 270;
                }
                #leaf3 {
                  margin-left: 265;
                }
                #id2 {
                  background-color: orange;
                  height: 20;
                  width: 30;
                  margin-left: 250;
                }
                #id3 {
                  background-color: lightgray;
                  height: 30;
                  width: 80;
                  margin-top: 3;
                  margin-left: 225;
                }
                #id4 {
                  background-color: lightgray;
                  height: 30;
                  width: 100;
                  margin-top: 3;
                  margin-left: 215;
                }
              </style>
            </head>
            <body>
              <div class=leaf id=leaf1></div>
              <div class=leaf id=leaf2></div>
              <div class=leaf id=leaf3></div>
              <div id=id2></div>
              <div id=id3></div>
              <div id=id4></div>
            </body>
            </html>"#;

        render(default_page.to_string(), &app);
    } else {
        let parsed_url = ParsedUrl::new(url.to_string());
        println!("parsed_url: {:?}", parsed_url);

        println!("----- receiving a response -----");
        let response = udp_response(&parsed_url);
        println!("{}", response);

        render(response, &app);
    }
}
