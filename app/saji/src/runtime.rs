use crate::alloc::string::ToString;
use crate::ast::Node;
use crate::ast::Program;
use alloc::format;
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec::Vec;
use core::borrow::Borrow;
use core::cell::RefCell;
//use hashbrown::hash_map::Entry;
use hashbrown::HashMap;
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

type VariableMap = HashMap<String, Option<RuntimeValue>>;

/// https://262.ecma-international.org/12.0/#sec-environment-records
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Environment {
    variables: VariableMap,
    outer: Option<Rc<RefCell<Environment>>>,
}

impl Environment {
    fn new(outer: Option<Rc<RefCell<Environment>>>) -> Self {
        Self {
            variables: VariableMap::new(),
            outer,
        }
    }

    pub fn get_variable(&self, name: String) -> Option<RuntimeValue> {
        match self.variables.get(&name) {
            Some(val) => val.clone(),
            None => {
                if let Some(p) = &self.outer {
                    p.borrow_mut().get_variable(name)
                } else {
                    None
                }
            }
        }
    }

    fn add_variable(&mut self, name: String, value: Option<RuntimeValue>) {
        self.variables.insert(name, value);
    }

    /*
    fn assign_variable(&mut self, name: String, value: Option<RuntimeValue>) {
        let entry = self.variables.entry(name.clone());
        match entry {
            Entry::Occupied(_) => {
                entry.insert(value);
            }
            Entry::Vacant(_) => {
                if let Some(p) = &self.outer {
                    p.borrow_mut().assign_variable(name, value);
                } else {
                    entry.insert(value);
                }
            }
        }
    }
    */
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
pub struct JsRuntime {
    pub global_variables: HashMap<String, Option<RuntimeValue>>,
    pub functions: Vec<Function>,
    pub env: Rc<RefCell<Environment>>,
}

impl JsRuntime {
    pub fn new() -> Self {
        Self {
            global_variables: HashMap::new(),
            functions: Vec::new(),
            env: Rc::new(RefCell::new(Environment::new(None))),
        }
    }

    fn eval(
        &mut self,
        node: &Option<Rc<Node>>,
        env: Rc<RefCell<Environment>>,
    ) -> Option<RuntimeValue> {
        let node = match node {
            Some(n) => n,
            None => return None,
        };

        match node.borrow() {
            Node::ExpressionStatement(expr) => return self.eval(&expr, env.clone()),
            Node::BlockStatement { body } => {
                let mut result: Option<RuntimeValue> = None;
                for stmt in body {
                    result = self.eval(&stmt, env.clone());
                }
                result
            }
            Node::ReturnStatement { argument } => {
                return self.eval(&argument, env.clone());
            }
            Node::FunctionDeclaration { id, params, body } => {
                let id = match self.eval(&id, env.clone()) {
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
                self.functions
                    .push(Function::new(id, params.to_vec(), cloned_body));
                None
            }
            Node::VariableDeclaration { declarations } => {
                for declaration in declarations {
                    self.eval(&declaration, env.clone());
                }
                None
            }
            Node::VariableDeclarator { id, init } => {
                if let Some(node) = id {
                    if let Node::Identifier(id) = node.borrow() {
                        let init = self.eval(&init, env.clone());
                        env.borrow_mut().add_variable(id.to_string(), init);
                        //self.global_variables.insert(id.to_string(), init);
                    }
                }
                None
            }
            Node::BinaryExpression {
                operator,
                left,
                right,
            } => {
                let left_value = match self.eval(&left, env.clone()) {
                    Some(value) => value,
                    None => return None,
                };
                let right_value = match self.eval(&right, env.clone()) {
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
                let env = Rc::new(RefCell::new(Environment::new(Some(env))));
                let callee_value = match self.eval(&callee, env.clone()) {
                    Some(value) => value,
                    None => return None,
                };

                let mut new_local_variables: VariableMap = VariableMap::new();

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
                    let name = match self.eval(&function.params[i], env.clone()) {
                        Some(value) => match value {
                            RuntimeValue::Number(n) => {
                                unimplemented!("id should be string but got {:?}", n)
                            }
                            RuntimeValue::StringLiteral(s) => s,
                        },
                        None => return None,
                    };

                    new_local_variables.insert(name, self.eval(&arguments[i], env.clone()));
                }

                // call function with arguments
                self.eval(&function.body.clone(), env.clone())
            }
            Node::Identifier(name) => {
                match env.borrow_mut().get_variable(name.to_string()) {
                    Some(v) => Some(v),
                    // First time to evaluate this identifier.
                    None => Some(RuntimeValue::StringLiteral(name.to_string())),
                }
                // Find a value from global variables.
                //for (var_name, var_value) in &self.global_variables {
                //if name == var_name && var_value.is_some() {
                //return var_value.clone();
                //}
                //}
            }
            Node::NumericLiteral(value) => Some(RuntimeValue::Number(*value)),
            Node::StringLiteral(value) => Some(RuntimeValue::StringLiteral(value.to_string())),
        }
    }

    pub fn execute(&mut self, program: &Program) {
        for node in program.body() {
            self.eval(&Some(node.clone()), self.env.clone());
        }
    }
}
