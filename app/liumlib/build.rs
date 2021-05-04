#![feature(array_map)]

use std::env;
use std::path::Path;
use std::process::Command;

fn get_object_name(s: &str) -> String {
    let mut v: Vec<&str> = s.split('.').collect();
    v.pop();
    v.push("o");
    v.join(".")
}

fn main() {
    let srcs = ["entry.S", "syscall.S"];
    let objs = srcs.map(|e| get_object_name(e)).to_vec();
    let out_dir = env::var("OUT_DIR").unwrap();
    let llvm_cc_path = env::var("LLVM_CC").unwrap();
    let llvm_ar_path = env::var("LLVM_AR").unwrap();

    // Build srcs
    for f in &srcs {
        let src = format!("src/{}", f);
        let dst = format!("{}/{}", out_dir, get_object_name(f));
        println!("{:?} => {:?}", src, dst);
        if !Command::new(Path::new(&llvm_cc_path))
            .args(&[
                src.as_str(),
                "-target",
                "x86_64-unknown-elf",
                "-c",
                "-o",
                dst.as_str(),
            ])
            .status()
            .expect("process failed to execute")
            .success()
        {
            panic!("Failed to build {}", f);
        }
    }
    // Create libliumos.a to be included at compile time.
    let mut ar_args = vec![String::from("crs"), String::from("libliumos.a")];
    for obj in objs {
        ar_args.push(obj);
    }
    if !Command::new(llvm_ar_path)
        .args(&ar_args)
        .current_dir(&Path::new(&out_dir))
        .status()
        .expect("process failed to execute")
        .success()
    {
        panic!("Failed to build ");
    }

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=liumos");
    println!(
        "cargo:rerun-if-changed={}",
        srcs.map(|s| {
            let mut t = "src/".to_string();
            t.push_str(s);
            t
        })
        .join(",")
    );
}
