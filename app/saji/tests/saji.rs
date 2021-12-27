#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;
use liumlib::*;
use saji::ast::Parser;
use saji::runtime::{Runtime, RuntimeValue};
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
    println!("Running {} tests in saji.rs", tests.len());
    for test in tests {
        test.run();
    }
}

#[macro_export]
macro_rules! run_test {
    ($input:expr, $expected_global_variables:expr) => {
        let lexer = Lexer::new($input);
        println!("lexer {:?}", lexer);

        let mut parser = Parser::new(lexer);
        let ast = parser.parse_ast();
        println!("---------------------------------");
        println!("ast {:?}", ast);
        println!("---------------------------------");

        let mut runtime = Runtime::new();
        runtime.execute(&ast);

        let result_global_variables = &runtime.global_variables;
        let defined_functions = &runtime.functions;
        println!("result global variables {:?}", result_global_variables);
        println!("result functions {:?}", defined_functions);

        assert!($expected_global_variables.len() == result_global_variables.len());
        if $expected_global_variables.len() == 0 {
            return;
        }
        assert!($expected_global_variables[0] == result_global_variables[0]);
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
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(3))));

    run_test!("var a = 1+2;".to_string(), &expected_global_variables);
}

#[test_case]
fn add_strings() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push((
        "a".to_string(),
        Some(RuntimeValue::StringLiteral("12".to_string())),
    ));

    run_test!(
        "var a = \"1\"+\"2\"".to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn add_number_and_string() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push((
        "a".to_string(),
        Some(RuntimeValue::StringLiteral("12".to_string())),
    ));

    run_test!("var a = 1+\"2\"".to_string(), &expected_global_variables);
}

#[test_case]
fn variable_declaration() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(42))));

    run_test!("var a = 42;".to_string(), &expected_global_variables);
}

#[test_case]
fn add_variable_and_number() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(1))));

    run_test!(
        r#"var a=1;
a+2"#
            .to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn variable_declaration_of_added_variable_and_number() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(1))));
    expected_global_variables.push(("b".to_string(), Some(RuntimeValue::Number(3))));

    run_test!(
        r#"var a=1;
var b=a+2"#
            .to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn white_spaces_and_new_lines() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(1))));
    expected_global_variables.push(("b".to_string(), Some(RuntimeValue::Number(3))));
    expected_global_variables.push(("c".to_string(), Some(RuntimeValue::Number(6))));
    expected_global_variables.push(("d".to_string(), Some(RuntimeValue::Number(42))));

    run_test!(
        r#"
var a =  1; var b=a+2;
var   c    = 3  + b;
    var d=42"#
            .to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn function_declaration() {
    let expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();

    run_test!(
        r#"
function test() {
    return 42;
}"#
        .to_string(),
        &expected_global_variables
    );
}

/*
#[test_case]
fn function_declaration_and_call() {
    let mut expected_global_variables = Vec::<(String, Option<RuntimeValue>)>::new();
    expected_global_variables.push(("a".to_string(), Some(RuntimeValue::Number(42))));

    run_test!(
        r#"
function test() {
    return 42;
}

var a=test();"#
            .to_string(),
        &expected_global_variables
    );
}
*/
