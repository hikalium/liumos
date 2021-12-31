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
pub struct Variable {
    name: String,
    value: Option<RuntimeValue>,
}

impl Variable {
    fn new(name: String, value: Option<RuntimeValue>) -> Self {
        Self { name, value }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Function {
    id: String,
    params: Vec<Option<Rc<Node>>>,
    body: Option<Rc<Node>>,
}

impl Function {
    fn new(id: String, params: Vec<Option<Rc<Node>>>, body: Option<Rc<Node>>) -> Self {
        Self { id, params, body }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Runtime {
    // name: String, value: Option<RuntimeValue>
    pub global_variables: Vec<(String, Option<RuntimeValue>)>,
    // pub global_variables: Vec<(String, Option<RuntimeValue>)>,
    pub functions: Vec<Function>,
    // id: String, params: Vec<Option<Rc<Node>>>, body: Option<Rc<Node>>
    //pub functions: Vec<(String, Vec<Option<Rc<Node>>>, Option<Rc<Node>>)>,
}

impl Runtime {
    pub fn new() -> Self {
        Self {
            global_variables: Vec::new(),
            functions: Vec::new(),
        }
    }

    fn eval(
        &mut self,
        node: &Option<Rc<Node>>,
        local_variables: Vec<(String, Option<RuntimeValue>)>,
    ) -> Option<RuntimeValue> {
        let node = match node {
            Some(n) => n,
            None => return None,
        };

        match node.borrow() {
            Node::ExpressionStatement(expr) => return self.eval(&expr, local_variables.clone()),
            Node::BlockStatement { body } => {
                let mut result: Option<RuntimeValue> = None;
                for stmt in body {
                    result = self.eval(&stmt, local_variables.clone());
                }
                result
            }
            Node::ReturnStatement { argument } => {
                return self.eval(&argument, local_variables.clone())
            }
            Node::FunctionDeclaration { id, params, body } => {
                let id = match self.eval(&id, local_variables.clone()) {
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
                //self.functions.push((id, params.to_vec(), cloned_body));
                self.functions
                    .push(Function::new(id, params.to_vec(), cloned_body));
                None
            }
            Node::VariableDeclaration { declarations } => {
                for declaration in declarations {
                    self.eval(&declaration, local_variables.clone());
                }
                None
            }
            Node::VariableDeclarator { id, init } => {
                let id = match self.eval(&id, local_variables.clone()) {
                    Some(value) => match value {
                        RuntimeValue::Number(n) => {
                            unimplemented!("id should be string but got {:?}", n)
                        }
                        RuntimeValue::StringLiteral(s) => s,
                    },
                    None => return None,
                };
                let init = self.eval(&init, local_variables.clone());
                self.global_variables.push((id, init));
                None
            }
            Node::BinaryExpression {
                operator,
                left,
                right,
            } => {
                let left_value = match self.eval(&left, local_variables.clone()) {
                    Some(value) => value,
                    None => return None,
                };
                let right_value = match self.eval(&right, local_variables.clone()) {
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
            Node::CallExpression { callee, arguments } => {
                let callee_value = match self.eval(&callee, local_variables.clone()) {
                    Some(value) => value,
                    None => return None,
                };

                let mut new_local_variables: Vec<(String, Option<RuntimeValue>)> = Vec::new();

                // find a function
                let function = {
                    let mut f: Option<Function> = None;
                    for func in &self.functions {
                        if callee_value == RuntimeValue::StringLiteral(func.id.to_string()) {
                            f = Some(func.clone());
                        }
                    }

                    match f {
                        Some(f) => f,
                        None => unimplemented!("function {:?} doesn't exist", callee),
                    }
                };

                // assign arguments to params as local variables
                assert!(arguments.len() == function.params.len());
                for i in 0..arguments.len() {
                    let name = match self.eval(&function.params[i], local_variables.clone()) {
                        Some(value) => match value {
                            RuntimeValue::Number(n) => {
                                unimplemented!("id should be string but got {:?}", n)
                            }
                            RuntimeValue::StringLiteral(s) => s,
                        },
                        None => return None,
                    };

                    new_local_variables
                        .push((name, self.eval(&arguments[i], local_variables.clone())));
                }
                new_local_variables.extend(local_variables.clone());

                // call function with arguments
                self.eval(&function.body.clone(), new_local_variables)
            }
            Node::Identifier(name) => {
                // Find a value from global varialbes.
                for (variable_name, runtime_value) in &self.global_variables {
                    if name == variable_name && runtime_value.is_some() {
                        return runtime_value.clone();
                    }
                }

                // Find a value from local varialbes.
                for (variable_name, runtime_value) in &local_variables {
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
            self.eval(&Some(node.clone()), Vec::new());
        }
    }
}
