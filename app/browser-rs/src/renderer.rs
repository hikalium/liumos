pub mod css;
pub mod dom;
pub mod tokenizer;

use alloc::string::String;
use dom::*;
use liumlib::*;
use tokenizer::*;

#[allow(dead_code)]
pub fn render(html: String) {
    println!("===== rendering start ===== ");
    println!("{}", html);

    let t = Tokenizer::new(html);
    let _root = Parser::new(t).construct_tree();
}
