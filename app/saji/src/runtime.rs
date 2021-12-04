use crate::ast::Program;
use alloc::string::String;
use crate::alloc::string::ToString;
use crate::ast::{Node, NodeKind};
use alloc::rc::Rc;
use alloc::format;
#[allow(unused_imports)]
use liumlib::*;

fn eval(node: &Option<Rc<Node>>) -> Result<u64, String> {
    let node = match node {
        Some(n) => n,
        None => return Err("node is None".to_string()),
    };

    match node.kind() {
        NodeKind::BinaryExpression => {
            let left_value = match eval(node.left()) {
                Ok(value) => value,
                Err(e) => return Err(e),
            };
            let right_value = match eval(node.right()) {
                Ok(value) => value,
                Err(e) => return Err(e),
            };
            return Ok(left_value + right_value);
        }
        NodeKind::NumericLiteral => {
            match node.value() {
                Some(value) => return Ok(*value),
                None => return Err("numericliteral should have a value".to_string()),
            }
        }
        _ => return Err(format!("unsupported node {:?}", node)),
    }
}

pub fn execute(program: &Program) {
    println!("----------------------------");

    for node in program.body() {
        match eval(&Some(node.clone())) {
            Ok(result) => println!("={:?}", result),
            Err(e) => println!("invalid expression {:?}", e),
        }
    }

    println!("----------------------------");
}
