#![no_std]
#![no_main]
#![feature(exclusive_range_pattern)]

mod ast;
mod runtime;
mod token;

extern crate alloc;

use crate::alloc::string::ToString;
use crate::ast::JsParser;
use crate::runtime::JsRuntime;
use crate::token::JsLexer;
use liumlib::*;

entry_point!(main);
fn main() {
    let args = env::args();
    if args.len() != 2 {
        println!("Usage: saji code");
        exit(0);
    }

    let lexer = JsLexer::new(args[1].to_string());
    println!("lexer {:?}", lexer);

    let mut parser = JsParser::new(lexer);
    let ast = parser.parse_ast();
    println!("ast {:?}", ast);

    let mut runtime = JsRuntime::new();
    runtime.execute(&ast);
}
