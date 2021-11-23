#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;

use alloc::rc::Rc;
use alloc::vec::Vec;
use browser_rs::renderer::ast::Node as AstNode;
use browser_rs::renderer::ast::{JsParser, Program};
use browser_rs::renderer::html_token::HtmlTokenizer;
use browser_rs::renderer::js_token::JsLexer;
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

#[macro_export]
macro_rules! run_test {
    ($html:literal, $expected:expr) => {
        use browser_rs::renderer::dom::*;

        let t1 = HtmlTokenizer::new(String::from($html));
        let mut p1 = HtmlParser::new(t1);
        let root = p1.construct_tree();
        let js = get_js_content(root.clone());

        assert!(js.is_some());
        let js_content = js.unwrap();
        println!("js_content {}", js_content);

        let t2 = JsLexer::new(js_content);
        let mut p2 = JsParser::new(t2);
        let program = p2.parse_ast();

        // Check nodes recursively.
        assert!(check_program($expected, &program));
    };
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

fn check_program(expected: &Program, actual: &Program) -> bool {
    for i in 0..expected.body().len() {
        if !node_equals(
            &Some(expected.body()[i].clone()),
            &Some(actual.body()[i].clone()),
        ) {
            return false;
        }
    }

    true
}

fn node_equals(expected: &Option<Rc<AstNode>>, actual: &Option<Rc<AstNode>>) -> bool {
    match expected {
        Some(expected_node) => match actual {
            Some(actual_node) => {
                if expected_node.kind() != actual_node.kind() {
                    println!("expected {:?} but actual {:?}", expected, actual);
                    return false;
                }

                return node_equals(expected_node.left(), actual_node.left())
                    && node_equals(expected_node.left(), actual_node.right());
            }
            None => {
                println!("expected {:?} but actual {:?}", expected, actual);
                return false;
            }
        },
        _ => {
            match actual {
                Some(_) => {
                    println!("expected {:?} but actual {:?}", expected, actual);
                    return false;
                }
                // Both nodes are None, return true.
                None => return true,
            }
        }
    }
}

#[test_case]
fn expression_statement() {
    let mut expected = Program::new();
    let mut body = Vec::new();
    body.push(Rc::new(AstNode::new_binary_expr(
        '+',
        Some(Rc::new(AstNode::new_num_literal(1))),
        Some(Rc::new(AstNode::new_num_literal(2))),
    )));
    expected.set_body(body);

    run_test!("<html><head><script>1+2;</script></head></html>", &expected);
}
