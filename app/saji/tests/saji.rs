#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::vec::Vec;
use liumlib::*;
use saji::ast::Parser;
use saji::runtime::{execute_for_test, RuntimeValue};
use saji::token::Lexer;

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

#[macro_export]
macro_rules! run_test {
    ($input:expr, $expected:expr) => {
        let lexer = Lexer::new($input);
        println!("lexer {:?}", lexer);

        let mut parser = Parser::new(lexer);
        let ast = parser.parse_ast();
        println!("ast {:?}", ast);

        let result = execute_for_test(&ast);

        assert!($expected[0] == result[0]);
    };
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[test_case]
fn add_numbers() {
    let mut expected = Vec::new();
    expected.push(RuntimeValue::Number(3));

    run_test!("1+2".to_string(), &expected);
}

#[test_case]
fn add_strings() {
    let mut expected = Vec::new();
    expected.push(RuntimeValue::StringLiteral("12".to_string()));

    run_test!("\"1\"+\"2\"".to_string(), &expected);
}

#[test_case]
fn add_number_and_string() {
    let mut expected = Vec::new();
    expected.push(RuntimeValue::StringLiteral("12".to_string()));

    run_test!("1+\"2\"".to_string(), &expected);
}

/*
#[test_case]
fn variable_declaration() {
    let mut expected = Vec::new();
    expected.push(RuntimeValue::StringLiteral("12".to_string()));

    run_test!("var a=42;".to_string(), &expected);
}
*/
