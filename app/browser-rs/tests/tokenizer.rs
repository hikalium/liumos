#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

use browser_rs::rendering::tokenizer::*;
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
    println!("Running {} tests", tests.len());
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

#[test_case]
fn no_input() {
    let mut t = Tokenizer::new(String::new());
    assert_eq!(t.next(), None);
}

#[test_case]
fn chars() {
    let mut t = Tokenizer::new(String::from("foo"));

    let mut expected = Vec::new();
    expected.push(Token::Char('f'));
    expected.push(Token::Char('o'));
    expected.push(Token::Char('o'));

    for e in expected {
        assert_eq!(t.next().expect("toknizer should have a next Token"), e);
    }
}

/*
#[test_case]
fn body() {
    let mut t = Tokenizer::new(String::from("<body></body>"));

    let mut expected = Vec::new();
    expected.push(Token::StartTag({ tag: String::from("body"), self_closing: false }));
    expected.push(Token::EndTag({ tag: String::from("body"), self_closing: false }));

    for e in expected {
        assert_eq!(t.next().expect("toknizer should have a next Token"), e);
    }
}
*/
