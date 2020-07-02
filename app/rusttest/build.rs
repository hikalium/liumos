use std::env;
use std::path::Path;
use std::process::Command;

fn get_object_name(s: &str) -> String {
    let mut v: Vec<&str> = s.split('.').collect();
    v.pop();
    v.push("o");
    return String::from(v.join("."));
}

fn main() {
    let srcs = ["hello.c", "syscall.S"];
    let out_dir = env::var("OUT_DIR").unwrap();

    for f in &srcs {
        let src = format!("src/{}", f);
        let dst = format!("{}/{}", out_dir, get_object_name(f));
        println!("{:?} => {:?}", src, dst);
        if !Command::new("/usr/local/Cellar/llvm/9.0.0_1/bin/clang")
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
    if !Command::new("/usr/local/Cellar/llvm/9.0.0_1/bin/llvm-ar")
        .args(&["crs", "libliumos.a", "hello.o", "syscall.o"])
        .current_dir(&Path::new(&out_dir))
        .status()
        .expect("process failed to execute")
        .success()
    {
        panic!("Failed to build ");
    }

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=liumos");
    println!("cargo:rerun-if-changed=src/hello.c,src/hello.S");
}
