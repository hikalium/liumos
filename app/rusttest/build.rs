use std::env;
use std::path::Path;
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    Command::new("/usr/local/Cellar/llvm/9.0.0_1/bin/clang")
        .args(&["src/hello.c", "-target", "x86_64-unknown-elf", "-c", "-o"])
        .arg(&format!("{}/hello.o", out_dir))
        .status()
        .unwrap();
    Command::new("/usr/local/Cellar/llvm/9.0.0_1/bin/clang")
        .args(&["src/syscall.S", "-target", "x86_64-unknown-elf", "-c", "-o"])
        .arg(&format!("{}/syscall.o", out_dir))
        .status()
        .unwrap();
    Command::new("/usr/local/Cellar/llvm/9.0.0_1/bin/llvm-ar")
        .args(&["crs", "libhello.a", "hello.o", "syscall.o"])
        .current_dir(&Path::new(&out_dir))
        .status()
        .unwrap();

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=hello");
    println!("cargo:rerun-if-changed=src/hello.c");
}
