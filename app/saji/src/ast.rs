//! https://github.com/estree/estree
//! https://astexplorer.net/

use crate::token::{Lexer, Token};
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone)]
pub struct Program {
    body: Vec<Rc<Node>>,
}

impl Program {
    pub fn new() -> Self {
        Self { body: Vec::new() }
    }

    pub fn set_body(&mut self, body: Vec<Rc<Node>>) {
        self.body = body;
    }

    pub fn body(&self) -> &Vec<Rc<Node>> {
        &self.body
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Node {
    /// https://github.com/estree/estree/blob/master/es5.md#expressionstatement
    ExpressionStatement(Option<Rc<Node>>),
    /// https://github.com/estree/estree/blob/master/es5.md#binaryexpression
    BinaryExpression {
        operator: char,
        left: Option<Rc<Node>>,
        right: Option<Rc<Node>>,
    },
    /// https://github.com/estree/estree/blob/master/es5.md#literal
    /// https://262.ecma-international.org/12.0/#prod-NumericLiteral
    NumericLiteral(u64),
    /// https://github.com/estree/estree/blob/master/es5.md#literal
    /// https://262.ecma-international.org/12.0/#prod-StringLiteral
    StringLiteral(String),
}

impl Node {
    pub fn new_binary_expr(
        operator: char,
        left: Option<Rc<Node>>,
        right: Option<Rc<Node>>,
    ) -> Self {
        Node::BinaryExpression {
            operator,
            left,
            right,
        }
    }

    pub fn new_expression_statement(expression: Option<Rc<Node>>) -> Self {
        Node::ExpressionStatement(expression)
    }
}

pub struct Parser {
    t: Lexer,
}

#[allow(dead_code)]
impl Parser {
    pub fn new(t: Lexer) -> Self {
        Self { t }
    }

    fn literal(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("token {:?}", t);

        match t {
            Token::Number(value) => return Some(Rc::new(Node::NumericLiteral(value))),
            Token::StringLiteral(s) => return Some(Rc::new(Node::StringLiteral(s))),
            _ => unimplemented!("token {:?} is not supported", t),
        }
    }

    /// AdditiveExpression ::= MultiplicativeExpression ( AdditiveOperator MultiplicativeExpression )*
    fn expression(&mut self) -> Option<Rc<Node>> {
        let left = self.literal();

        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("token {:?}", t);

        match t {
            Token::Punctuator(c) => match c {
                '+' | '-' => Some(Rc::new(Node::new_binary_expr(c, left, self.literal()))),
                _ => unimplemented!("`Punctuator` token with {} is not supported", c),
            },
            _ => None,
        }
    }

    /// VariableDeclarationList ::= VariableDeclaration ( "," VariableDeclaration )*
    /// VariableDeclaration ::= Identifier ( Initialiser )?
    fn variable_declaration(&mut self) -> Option<Rc<Node>> {
        None
    }

    /// https://262.ecma-international.org/12.0/#prod-Statement
    /// Statement ::= ExpressionStatement
    ///             | VariableStatement
    ///
    /// VariableStatement ::= "var" VariableDeclarationList ( ";" )?
    /// ExpressionStatement ::= Expression ( ";" )?
    fn statement(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.peek() {
            Some(t) => t,
            None => return None,
        };

        match t {
            Token::Keyword(_keyword) => {
                return self.variable_declaration();
            }
            _ => {
                let expr = self.expression();

                let t = match self.t.next() {
                    Some(token) => token,
                    None => return Some(Rc::new(Node::new_expression_statement(expr))),
                };

                println!("token {:?}", t);

                match t {
                    Token::Punctuator(c) => match c {
                        ';' => return Some(Rc::new(Node::new_expression_statement(expr))),
                        _ => unimplemented!("`Punctuator` token with {} is not supported", c),
                    },
                    _ => return Some(Rc::new(Node::new_expression_statement(expr))),
                }
            }
        }
    }

    pub fn parse_ast(&mut self) -> Program {
        let mut program = Program::new();

        // interface Program <: Node {
        //   type: "Program";
        //   body: [ Directive | Statement ];
        // }
        let mut body = Vec::new();

        loop {
            match self.statement() {
                Some(node) => body.push(node),
                None => break,
            }
        }

        program.set_body(body);
        program
    }
}
