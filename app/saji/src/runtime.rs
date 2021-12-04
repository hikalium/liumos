use crate::ast::Program;
#[allow(unused_imports)]
use liumlib::*;

pub fn execute(program: &Program) {
    for stmt in program.body() {
        println!("statement {:?}", stmt);
    }
}
