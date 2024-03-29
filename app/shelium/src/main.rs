#![no_std]
#![no_main]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;
use core::mem::size_of;
use liumlib::*;

const SHELIUM_VERSION: &str = env!("CARGO_PKG_VERSION");
const SHELIUM_GIT_HASH: &str = env!("GIT_HASH");

fn run_command(line: &str) {
    if line == "version" {
        println!("shelium {} {}", SHELIUM_VERSION, SHELIUM_GIT_HASH);
        return;
    }
    if line == "ls" {
        let fd = open(".", 0, 0);
        if fd.is_none() {
            println!("open failed");
            return;
        }
        let fd = fd.unwrap();
        let mut buf = [0; 512];
        let p = buf.as_mut_ptr();
        let mut files = Vec::new();
        struct FileNameAndId {
            name: String,
            id: u64,
        }
        loop {
            let bytes_read = getdents64(&fd, &mut buf);
            if bytes_read < 0 {
                println!("getdents failed");
                return;
            }
            if bytes_read == 0 {
                break;
            }
            let mut cur = 0;
            while cur < bytes_read as usize {
                // TODO: check bytes_read before reading bytes
                let de: &DirectoryEntry = unsafe { &*(p.add(cur) as *mut DirectoryEntry) };
                let file_name_bytes = &buf[(size_of::<DirectoryEntry>() + cur)..(de.size() + cur)];
                let null_terminator_pos = file_name_bytes.iter().position(|&v| v == 0);
                let file_name_bytes = match null_terminator_pos {
                    Some(idx) => &file_name_bytes[..idx],
                    None => file_name_bytes,
                };
                let file_name = String::from_utf8(file_name_bytes.to_vec()).unwrap();
                cur += de.size();
                if file_name.starts_with('.') {
                    continue;
                }
                files.push(FileNameAndId {
                    name: file_name,
                    id: de.inode(),
                })
            }
        }
        files.sort_by(|l, r| l.name.cmp(&r.name));
        for f in files {
            println!("{:8} {}", f.id, f.name);
        }
        return;
    }
    if line == "test_mmap" {
        let addr = alloc_page();
        assert!(!addr.is_null());
        println!("allocated: {:?}", addr);
        assert!(free_page(addr) == 0);
        println!("freed: {:?}", addr);
        return;
    }
    println!("Not a command or a file: {}", line);
}

entry_point!(main);
fn main() {
    let arg_version = "-v".to_string();
    for arg in env::args() {
        if arg_version == *arg {
            run_command("version");
            exit(0);
        }
    }

    println!("Welcome to shelium, a simple shell for liumOS written in Rust!");
    let mut line = String::with_capacity(32);

    loop {
        let c = getchar();
        putchar(c);
        if c == b'\n' {
            println!("{:?}", line);
            if line == "exit" {
                break;
            }
            run_command(&line);
            line.truncate(0);
            continue;
        }
        line.push(c as char);
    }
}
