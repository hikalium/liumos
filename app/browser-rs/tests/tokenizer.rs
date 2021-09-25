#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;

use browser_rs::renderer::tokenizer::*;
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
    println!("Running {} tests in tokenizer.rs", tests.len());
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
            assert_eq!(token, e, "\n\nexpected {:?} but got {:?}", e, token);
        }
    };
}

#[test_case]
fn no_input() {
    run_test!("");
}

#[test_case]
fn chars() {
    run_test!("foo", Token::Char('f'), Token::Char('o'), Token::Char('o'));
}

#[test_case]
fn body() {
    run_test!(
        "<body></body>",
        Token::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::EndTag {
            tag: String::from("body"),
            self_closing: false,
        }
    );
}

#[test_case]
fn BODY() {
    run_test!(
        "<BODY></BODY>",
        Token::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::EndTag {
            tag: String::from("body"),
            self_closing: false
        }
    );
}

#[test_case]
fn br() {
    run_test!(
        "<br/>",
        Token::StartTag {
            tag: String::from("br"),
            self_closing: true,
            attributes: Vec::new(),
        }
    );
}

#[test_case]
fn simple_page() {
    run_test!(
        "<html><body>abc</body></html>",
        Token::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::Char('a'),
        Token::Char('b'),
        Token::Char('c'),
        Token::EndTag {
            tag: String::from("body"),
            self_closing: false,
        },
        Token::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn link() {
    let mut attributes = Vec::new();
    let mut a1 = Attribute::new();
    a1.set_name_and_value("rel".to_string(), "stylesheet".to_string());
    attributes.push(a1);
    let mut a2 = Attribute::new();
    a2.set_name_and_value("href".to_string(), "styles.css".to_string());
    attributes.push(a2);

    run_test!(
        "<html><head><link rel='stylesheet' href='styles.css'></head></html>",
        Token::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::StartTag {
            tag: String::from("head"),
            self_closing: false,
            attributes: Vec::new(),
        },
        Token::StartTag {
            tag: String::from("link"),
            self_closing: false,
            attributes,
        },
        Token::EndTag {
            tag: String::from("head"),
            self_closing: false,
        },
        Token::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}
