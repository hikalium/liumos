//! https://github.com/estree/estree
//! https://astexplorer.net/

use crate::token::{JsLexer, JsToken};
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

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Node {
    /// https://github.com/estree/estree/blob/master/es5.md#expressionstatement
    ExpressionStatement,
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
}

pub struct JsParser {
    t: JsLexer,
}

#[allow(dead_code)]
impl JsParser {
    pub fn new(t: JsLexer) -> Self {
        Self { t }
    }

    fn literal(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };

        println!("token {:?}", t);

        match t {
            JsToken::Number(value) => return Some(Rc::new(Node::NumericLiteral(value))),
            JsToken::StringLiteral(s) => return Some(Rc::new(Node::StringLiteral(s))),
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
            JsToken::Punctuator(c) => match c {
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
        let expr = self.expression();

        let t = match self.t.next() {
            Some(token) => token,
            None => return expr,
        };

        println!("token {:?}", t);

        match t {
            JsToken::Punctuator(c) => match c {
                ';' => return expr,
                _ => unimplemented!("`Punctuator` token with {} is not supported", c),
            },
            _ => return expr,
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
