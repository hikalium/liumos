mod parser;
mod tokenizer;

use alloc::string::String;
use liumlib::*;
use tokenizer::*;

#[allow(dead_code)]
pub fn render(html: String) {
    println!("===== rendering start ===== ");
    println!("{}", html);

    let tokenizer = Tokenizer::new(html).tokens();
    println!("{:?}", tokenizer);
}
