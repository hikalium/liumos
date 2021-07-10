#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

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
    ($html:literal, $( $node:expr ),*) => {
        let t = Tokenizer::new(String::from($html));

        let mut p = Parser::new(t);
        let tree = p.construct_tree();

        let mut queue = Vec::new();

        queue.push(tree.root());
        /*
        let mut sibiling = root.next_sibling();

        while sibiling.is_some() {
            queue.push(sibiling);
            sibiling = sibiling.next_sibling();
        }

        // TODO: fix duplicated children
        let mut child = root.first_child();
        while child.is_some() {
            queue.push(child);
            child = child.next_sibling();
        }
        */

        $(
            let n = queue.remove(0);

            assert_eq!($node, n);

            /*
            while sibiling.is_some() {
                queue.push(sibiling);
                sibiling = sibiling.next_sibling();
            }
            */

            // TODO: add children to queue.
        )*
    };
}

#[test_case]
fn no_input() {
    run_test!("", Node::Document);
}

/*
#[test_case]
fn html() {
    run_test!(
        "<html></html>",
        Node::Document,
        Node::Element(Element::HtmlElement(HtmlElementImpl::new()))
    );
}
*/
