use crate::alloc::string::ToString;
use crate::ast::Node;
use crate::ast::Program;
use alloc::format;
use alloc::rc::Rc;
use alloc::string::String;
use core::borrow::Borrow;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone)]
enum Object {
    Number(u64),
    StringLiteral(String),
}

impl Object {
    fn to_string(&self) -> String {
        match self {
            Object::Number(value) => format!("{}", value),
            Object::StringLiteral(value) => value.to_string(),
        }
    }
}

fn eval(node: &Option<Rc<Node>>) -> Result<Object, String> {
    let node = match node {
        Some(n) => n,
        None => return Err("node is None".to_string()),
    };

    match node.borrow() {
        Node::BinaryExpression {
            operator,
            left,
            right,
        } => {
            let left_value = match eval(&left) {
                Ok(value) => value,
                Err(e) => return Err(e),
            };
            let right_value = match eval(&right) {
                Ok(value) => value,
                Err(e) => return Err(e),
            };

            // https://tc39.es/ecma262/multipage/ecmascript-language-expressions.html#sec-applystringornumericbinaryoperator
            if *operator == '+' {
                if let (Object::Number(left_num), Object::Number(right_num)) =
                    (left_value.clone(), right_value.clone())
                {
                    return Ok(Object::Number(left_num + right_num));
                }
                return Ok(Object::StringLiteral(
                    left_value.to_string() + &right_value.to_string(),
                ));
            } else {
                return Err(format!("unsupported operator {:?}", operator));
            }
        }
        Node::NumericLiteral(value) => Ok(Object::Number(*value)),
        Node::StringLiteral(value) => Ok(Object::StringLiteral(value.to_string())),
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
