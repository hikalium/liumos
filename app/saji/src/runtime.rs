use crate::alloc::string::ToString;
use crate::ast::Node;
use crate::ast::Program;
use alloc::format;
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec::Vec;
use core::borrow::Borrow;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RuntimeValue {
    Number(u64),
    StringLiteral(String),
}

impl RuntimeValue {
    fn to_string(&self) -> String {
        match self {
            RuntimeValue::Number(value) => format!("{}", value),
            RuntimeValue::StringLiteral(value) => value.to_string(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Runtime {
    pub global_variables: Vec<(String, Option<RuntimeValue>)>,
    pub functions: Vec<(String, Option<Rc<Node>>)>,
}

impl Runtime {
    pub fn new() -> Self {
        Self {
            global_variables: Vec::new(),
            functions: Vec::new(),
        }
    }

    fn eval(&mut self, node: &Option<Rc<Node>>) -> Option<RuntimeValue> {
        let node = match node {
            Some(n) => n,
            None => return None,
        };

        match node.borrow() {
            Node::ExpressionStatement(expr) => return self.eval(&expr),
            Node::BlockStatement { body } => {
                for stmt in body {
                    self.eval(&stmt);
                }
                None
            }
            Node::ReturnStatement { argument: _ } => None,
            Node::FunctionDeclaration {
                id,
                params: _,
                body,
            } => {
                let id = match self.eval(&id) {
                    Some(value) => match value {
                        RuntimeValue::Number(n) => {
                            unimplemented!("id should be string but got {:?}", n)
                        }
                        RuntimeValue::StringLiteral(s) => s,
                    },
                    None => return None,
                };
                let cloned_body = match body {
                    Some(b) => Some(b.clone()),
                    None => None,
                };
                self.functions.push((id, cloned_body));
                None
            }
            Node::VariableDeclaration { declarations } => {
                for declaration in declarations {
                    self.eval(&declaration);
                }
                None
            }
            Node::VariableDeclarator { id, init } => {
                let id = match self.eval(&id) {
                    Some(value) => match value {
                        RuntimeValue::Number(n) => {
                            unimplemented!("id should be string but got {:?}", n)
                        }
                        RuntimeValue::StringLiteral(s) => s,
                    },
                    None => return None,
                };
                let init = self.eval(&init);
                self.global_variables.push((id, init));
                None
            }
            Node::BinaryExpression {
                operator,
                left,
                right,
            } => {
                let left_value = match self.eval(&left) {
                    Some(value) => value,
                    None => return None,
                };
                let right_value = match self.eval(&right) {
                    Some(value) => value,
                    None => return None,
                };

                // https://tc39.es/ecma262/multipage/ecmascript-language-expressions.html#sec-applystringornumericbinaryoperator
                if *operator == '+' {
                    if let (RuntimeValue::Number(left_num), RuntimeValue::Number(right_num)) =
                        (left_value.clone(), right_value.clone())
                    {
                        return Some(RuntimeValue::Number(left_num + right_num));
                    }
                    return Some(RuntimeValue::StringLiteral(
                        left_value.to_string() + &right_value.to_string(),
                    ));
                } else {
                    return None;
                }
            }
            Node::CallExpression {
                callee,
                arguments: _,
            } => {
                let callee_value = match self.eval(&callee) {
                    Some(value) => value,
                    None => return None,
                };

                let function_body: Option<Rc<Node>> = {
                    let mut body = None;

                    for (function_name, function_body) in &self.functions {
                        if callee_value == RuntimeValue::StringLiteral(function_name.to_string()) {
                            body = match function_body {
                                Some(b) => Some(b.clone()),
                                None => None,
                            };
                        }
                    }

                    body
                };

                return self.eval(&function_body);
            }
            Node::Identifier(name) => {
                for (variable_name, runtime_value) in &self.global_variables {
                    if name == variable_name && runtime_value.is_some() {
                        return runtime_value.clone();
                    }
                }

                // First time to evaluate this identifier.
                Some(RuntimeValue::StringLiteral(name.to_string()))
            }
            Node::NumericLiteral(value) => Some(RuntimeValue::Number(*value)),
            Node::StringLiteral(value) => Some(RuntimeValue::StringLiteral(value.to_string())),
        }
    }

    pub fn execute(&mut self, program: &Program) {
        for node in program.body() {
            self.eval(&Some(node.clone()));
        }
    }
}
