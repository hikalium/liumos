#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;

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
    println!("Running {} tests in dom.rs", tests.len());
    for test in tests {
        test.run();
    }
}

#[macro_export]
macro_rules! run_test {
    ($html:literal) => {
        use browser_rs::renderer::dom::*;

        let t = Tokenizer::new(String::from($html));

        let mut p = Parser::new(t);
        let root = p.construct_tree();
        let style = get_style_content(root.clone());

        println!("style: {:?}", style);
    };
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[test_case]
fn background_color() {
    // root (Document)
    // └── html
    //     └── head
    //         └── style
    //     └── body
    run_test!("<html><head><style>h1{background-color:red;}</style></head></html>");
}
