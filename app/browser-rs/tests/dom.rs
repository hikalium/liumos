#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::rc::Rc;
use alloc::string::String;
use core::cell::RefCell;

use browser_rs::parser::dom::*;
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
    ($html:literal, $expected_root:expr) => {
        let t = Tokenizer::new(String::from($html));

        let mut p = Parser::new(t);
        let root = p.construct_tree();

        assert_eq!($expected_root, root);

        // TODO: check children
    };
}

#[test_case]
fn no_input() {
    let root = Rc::new(RefCell::new(Node::new(NodeKind::Document)));
    run_test!("", root);
}

#[test_case]
fn html() {
    let root = Rc::new(RefCell::new(Node::new(NodeKind::Document)));
    let html = Rc::new(RefCell::new(Node::new(NodeKind::Element(
        Element::HtmlElement(HtmlElementImpl::new()),
    ))));
    //root.parent = &html;
    //root.first_child = html;
    {
        root.borrow_mut().last_child = Some(html);
    }
    run_test!("<html></html>", root);
}
