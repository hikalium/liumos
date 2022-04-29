#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

//use core::cell::RefCell;
use crate::alloc::string::ToString;
use alloc::rc::Rc;
use alloc::string::String;
use core::borrow::Borrow;
use hashbrown::HashMap;
use liumlib::*;
use saji::ast::{JsParser, Node};
use saji::runtime::{JsRuntime, RuntimeValue};
use saji::token::JsLexer;

#[macro_export]
macro_rules! run_test {
    ($input:expr, $expected_global_variables:expr) => {
        let lexer = JsLexer::new($input);
        println!("lexer {:?}", lexer);

        let mut parser = JsParser::new(lexer);
        let ast = parser.parse_ast();
        println!("---------------------------------");
        for n in ast.body() {
            print_ast(&Some(n.clone()), 0);
        }
        println!("---------------------------------");

        let mut runtime = JsRuntime::new();
        runtime.execute(&ast);

        //let result_global_variables = &runtime.global_variables;
        //let defined_functions = &runtime.functions;
        //println!("result global variables {:?}", result_global_variables);
        //println!("expected global variables {:?}", $expected_global_variables);
        //println!("result functions {:?}", defined_functions);

        for (name, value) in $expected_global_variables {
            assert!((*runtime.env.borrow_mut()).get_variable(name.to_string()) == *value);
        }

        //assert!($expected_global_variables.len() == result_global_variables.len());
        //if $expected_global_variables.len() == 0 {
        //return;
        //}
    };
}

fn print_ast(node: &Option<Rc<Node>>, depth: usize) {
    let n = match node {
        Some(n) => n,
        None => return,
    };

    match n.borrow() {
        Node::ExpressionStatement(stmt) => {
            println!("{}ExpressionStatement", "  ".repeat(depth));
            print_ast(&stmt, depth + 1);
        }
        Node::BlockStatement { body } => {
            println!("{}BlockStatement", "  ".repeat(depth));
            for node in body {
                print_ast(&node, depth + 1);
            }
        }
        Node::ReturnStatement { argument } => {
            println!("{}ReturnStatement", "  ".repeat(depth));
            print_ast(&argument, depth + 1);
        }
        Node::FunctionDeclaration { id, params, body } => {
            println!("{}FunctionDeclaration", "  ".repeat(depth));
            print_ast(&id, depth + 1);
            for param in params {
                print_ast(&param, depth + 1);
            }
            print_ast(&body, depth + 1);
        }
        Node::VariableDeclaration { declarations } => {
            println!("{}VariableDeclaration", "  ".repeat(depth));
            for decl in declarations {
                print_ast(&decl, depth + 1);
            }
        }
        Node::VariableDeclarator { id, init } => {
            println!("{}VariableDeclarator", "  ".repeat(depth));
            print_ast(&id, depth + 1);
            print_ast(&init, depth + 1);
        }
        Node::BinaryExpression {
            operator,
            left,
            right,
        } => {
            print!("{}", "  ".repeat(depth));
            println!("BinaryExpression: {}", operator);
            print_ast(&left, depth + 1);
            print_ast(&right, depth + 1);
        }
        Node::CallExpression { callee, arguments } => {
            println!("{}CallExpression", "  ".repeat(depth));
            print_ast(&callee, depth + 1);
            for arg in arguments {
                print_ast(&arg, depth + 1);
            }
        }
        Node::Identifier(i) => {
            print!("{}", "  ".repeat(depth));
            println!("Identifier: {}", i);
        }
        Node::NumericLiteral(n) => {
            print!("{}", "  ".repeat(depth));
            println!("NumericLiteral: {}", n);
        }
        Node::StringLiteral(s) => {
            print!("{}", "  ".repeat(depth));
            println!("StringLiteral: {}", s);
        }
    }
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[test_case]
fn add_numbers() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(3)));

    run_test!("var a = 1+2;".to_string(), &expected_global_variables);
}

#[test_case]
fn add_strings() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert(
        "a".to_string(),
        Some(RuntimeValue::StringLiteral("12".to_string())),
    );

    run_test!(
        "var a = \"1\"+\"2\"".to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn add_number_and_string() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert(
        "a".to_string(),
        Some(RuntimeValue::StringLiteral("12".to_string())),
    );

    run_test!("var a = 1+\"2\"".to_string(), &expected_global_variables);
}

#[test_case]
fn variable_declaration() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(42)));

    run_test!("var a = 42;".to_string(), &expected_global_variables);
}

#[test_case]
fn add_variable_and_number() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(1)));

    run_test!(
        r#"var a=1;
a+2"#
            .to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn variable_declaration_of_added_variable_and_number() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(1)));
    expected_global_variables.insert("b".to_string(), Some(RuntimeValue::Number(3)));

    run_test!(
        r#"var a=1;
var b=a+2"#
            .to_string(),
        &expected_global_variables
    );
}

/*
#[test_case]
fn white_spaces_and_new_lines() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(1)));
    expected_global_variables.insert("b".to_string(), Some(RuntimeValue::Number(3)));
    expected_global_variables.insert("c".to_string(), Some(RuntimeValue::Number(6)));
    expected_global_variables.insert("d".to_string(), Some(RuntimeValue::Number(42)));

    run_test!(
        r#"
var a =  1; var b=a+2;
var   c    = 3  + b;
    var d=42"#
            .to_string(),
        &expected_global_variables
    );
}
*/

#[test_case]
fn function_declaration() {
    let expected_global_variables: HashMap<String, Option<RuntimeValue>> = HashMap::new();

    run_test!(
        r#"
function test() {
    return 42;
}"#
        .to_string(),
        &expected_global_variables
    );
}

#[test_case]
fn function_call() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(42)));

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

#[test_case]
fn function_declaration_with_params() {
    let expected_global_variables: HashMap<String, Option<RuntimeValue>> = HashMap::new();

    run_test!(
        r#"
function test(param1, param2) {
    return 42;
}"#
        .to_string(),
        &expected_global_variables
    );
}

/*
#[test_case]
fn function_call_with_arguments() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(3)));

    run_test!(
        r#"
function test(param1, param2) {
    return param1 + param2;
}

var a=test(1, 2);
"#
        .to_string(),
        &expected_global_variables
    );
}
*/

/*
#[test_case]
fn override_with_local_variable() {
    let mut expected_global_variables = HashMap::new();
    expected_global_variables.insert("a".to_string(), Some(RuntimeValue::Number(2)));
    expected_global_variables.insert("b".to_string(), Some(RuntimeValue::Number(1)));

    run_test!(
        r#"
var b=1;
function test() {
    var b=2;
    return b;
}

var a=test();
"#
        .to_string(),
        &expected_global_variables
    );
}
*/
