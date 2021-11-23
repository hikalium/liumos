#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use alloc::string::String;

use browser_rs::renderer::ast::JsParser;
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
    ($html:literal, $expected_style:expr) => {
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
        let _program = p2.parse_ast();
    };
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

#[test_case]
fn script() {
    run_test!(
        "<html><head><script>1+2</script></head></html>",
        JsToken::Number(42)
    );
}
