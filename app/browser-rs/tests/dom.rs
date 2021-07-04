#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

use browser_rs::parser::tokenizer::*;
use liumlib::*;

#[cfg(test)]
pub trait Testable {
    fn run(&self) -> ();
}

#[cfg(test)]
impl<T> Testable for T
where
    T: Fn(),
{
    fn run(&self) {
        print!("{} ...\t", core::any::type_name::<T>());
        self();
        println!("[ok]");
    }
}

#[cfg(test)]
pub fn test_runner(tests: &[&dyn Testable]) {
    println!("Running {} tests in dom.rs", tests.len());
    for test in tests {
        test.run();
    }
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[macro_export]
macro_rules! run_test {
    ($html:literal) => {
        let mut t = Tokenizer::new(String::from($html));
        assert_eq!(t.next(), None);
    };
    ($html:literal, $( $token:expr ),*) => {
        let mut t = Tokenizer::new(String::from($html));

        let mut expected = Vec::new();
        $(
            expected.push($token);
        )*

        for e in expected {
            let token = t.next().expect("tokenizer should have a next Token");
            assert_eq!(token, e, "expected {:?} but got {:?}", e, token);
        }
    };
}

#[test_case]
fn br() {
    run_test!(
        "<br/>",
        Token::StartTag {
            tag: String::from("br"),
            self_closing: true,
        }
    );
}
