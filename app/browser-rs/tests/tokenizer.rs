#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

use liumlib::*;

#[cfg(test)]
fn test_runner(tests: &[&dyn Fn()]) {
    println!("Running {} tests in tokenizer.rs", tests.len());
    for test in tests {
        test();
        println!("[ok]");
    }
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[test_case]
fn basic() {
    print!("basic test ");
    assert_eq!(1, 1);
}
