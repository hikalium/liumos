//! https://github.com/estree/estree
//! https://astexplorer.net/

use crate::renderer::js::js_token::{JsLexer, JsToken};
use alloc::rc::Rc;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
pub struct Program {
    body: Vec<Rc<Node>>,
}

#[allow(dead_code)]
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
pub enum NodeKind {
    /// https://github.com/estree/estree/blob/master/es5.md#expressionstatement
    ExpressionStatement,
    /// https://github.com/estree/estree/blob/master/es5.md#binaryexpression
    BinaryExpression,
    /// https://github.com/estree/estree/blob/master/es5.md#literal
    /// https://262.ecma-international.org/12.0/#prod-NumericLiteral
    NumericLiteral,
    /// https://github.com/estree/estree/blob/master/es5.md#literal
    /// https://262.ecma-international.org/12.0/#prod-StringLiteral
    StringLiteral,
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct Node {
    kind: NodeKind,
    left: Option<Rc<Node>>,
    right: Option<Rc<Node>>,
    value: Option<u64>,
    operator: Option<char>,
}

#[allow(dead_code)]
impl Node {
    fn new() -> Self {
        Self {
            kind: NodeKind::ExpressionStatement,
            left: None,
            right: None,
            value: None,
            operator: None,
        }
    }

    pub fn new_num_literal(value: u64) -> Self {
        Self {
            kind: NodeKind::NumericLiteral,
            left: None,
            right: None,
            value: Some(value),
            operator: None,
        }
    }

    pub fn new_binary_expr(
        operator: char,
        left: Option<Rc<Node>>,
        right: Option<Rc<Node>>,
    ) -> Self {
        Self {
            kind: NodeKind::BinaryExpression,
            left,
            right,
            value: None,
            operator: Some(operator),
        }
    }

    pub fn kind(&self) -> &NodeKind {
        &self.kind
    }

    pub fn left(&self) -> &Option<Rc<Node>> {
        &self.left
    }

    pub fn right(&self) -> &Option<Rc<Node>> {
        &self.right
    }
}

#[allow(dead_code)]
pub struct JsParser {
    t: JsLexer,
}

#[allow(dead_code)]
impl JsParser {
    pub fn new(t: JsLexer) -> Self {
        Self { t }
    }

    fn number(&mut self) -> Option<Rc<Node>> {
        let n = match self.t.next() {
            Some(node) => node,
            None => return None,
        };

        match n {
            JsToken::Number(value) => return Some(Rc::new(Node::new_num_literal(value))),
            _ => unimplemented!("token {:?} is not supported", n),
        }
    }

    /// AdditiveExpression ::= MultiplicativeExpression ( AdditiveOperator MultiplicativeExpression )*
    fn expression(&mut self) -> Option<Rc<Node>> {
        let left = self.number();

        let n = match self.t.next() {
            Some(node) => node,
            None => return None,
        };

        match n {
            JsToken::Punctuator(c) => match c {
                '+' | '-' => Some(Rc::new(Node::new_binary_expr(c, left, self.number()))),
                _ => unimplemented!("`Punctuator` token with {} is not supported", c),
            },
            _ => None,
        }
    }

    /// https://262.ecma-international.org/12.0/#prod-Statement
    /// Statement ::= ExpressionStatement
    fn statement(&mut self) -> Option<Rc<Node>> {
        let expr = self.expression();

        let n = match self.t.next() {
            Some(node) => node,
            None => return None,
        };

        match n {
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
