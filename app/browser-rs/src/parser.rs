pub mod dom;
pub mod tokenizer;

use alloc::string::String;
use liumlib::*;
use tokenizer::*;

#[allow(dead_code)]
pub fn render(html: String) {
    println!("===== rendering start ===== ");
    println!("{}", html);

    let _tokenizer = Tokenizer::new(html);
}
