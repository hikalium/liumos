#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;

use browser_rs::renderer::css_token::CssTokenizer;
use browser_rs::renderer::cssom::*;
use browser_rs::renderer::html_token::HtmlTokenizer;
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

        let t = HtmlTokenizer::new(String::from($html));
        let mut p = HtmlParser::new(t);
        let root = p.construct_tree();
        let style = get_style_content(root.clone());

        assert!(style.is_some());
        let style_content = style.unwrap();

        let t2 = CssTokenizer::new(style_content);
        let mut p2 = CssParser::new(t2);
        let cssom = p2.parse_stylesheet();
        assert_eq!($expected_style, cssom);
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
    let mut decl = Declaration::new();
    decl.set_property("background-color".to_string());
    decl.set_value("red".to_string());

    let mut decls = Vec::new();
    decls.push(decl);

    let mut rule = QualifiedRule::new();
    rule.set_selector(Selector::TypeSelector("h1".to_string()));
    rule.set_declarations(decls);

    let mut rules = Vec::new();
    rules.push(rule);

    let mut expected = StyleSheet::new();
    expected.set_rules(rules);

    run_test!(
        "<html><head><style>h1{background-color:red;}</style></head></html>",
        expected
    );
}

/*
#[test_case]
fn class_selector() {
    let mut decl = Declaration::new();
    decl.set_property("background-color".to_string());
    decl.set_value("red".to_string());

    let mut decls = Vec::new();
    decls.push(decl);

    let mut rule = QualifiedRule::new();
    rule.set_selector(".class".to_string());
    rule.set_declarations(decls);

    let mut rules = Vec::new();
    rules.push(rule);

    let mut expected = StyleSheet::new();
    expected.set_rules(rules);

    run_test!(
        "<html><head><style>.class{background-color:red;}</style></head></html>",
        expected
    );
}
*/

/*
#[test_case]
fn id_selector() {
    let mut decl = Declaration::new();
    decl.set_property("background-color".to_string());
    decl.set_value("red".to_string());

    let mut decls = Vec::new();
    decls.push(decl);

    let mut rule = QualifiedRule::new();
    rule.set_selector(Selector::IdSelector("id".to_string()));
    rule.set_declarations(decls);

    let mut rules = Vec::new();
    rules.push(rule);

    let mut expected = StyleSheet::new();
    expected.set_rules(rules);

    run_test!(
        "<html><head><style>#id{background-color:red;}</style></head></html>",
        expected
    );
}
*/
