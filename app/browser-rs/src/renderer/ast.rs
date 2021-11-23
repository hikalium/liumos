//! https://github.com/estree/estree
//! https://astexplorer.net/

use crate::renderer::js_token::{JsLexer, JsToken};
use alloc::boxed::Box;
use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
pub struct Program {
    body: Vec<Box<Node>>,
}

impl Program {
    pub fn new() -> Self {
        Self { body: Vec::new() }
    }

    fn set_body(&mut self, body: Vec<Box<Node>>) {
        self.body = body;
    }
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
struct Node {
    kind: NodeKind,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
    num: Option<u64>,
    operator: Option<String>,
}

#[allow(dead_code)]
impl Node {
    fn new() -> Self {
        Self {
            kind: NodeKind::ExpressionStatement,
            left: None,
            right: None,
            num: None,
            operator: None,
        }
    }
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
enum NodeKind {
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
pub struct JsParser {
    t: JsLexer,
    current: Option<JsToken>,
}

#[allow(dead_code)]
impl JsParser {
    pub fn new(t: JsLexer) -> Self {
        Self { t, current: None }
    }

    fn update_current_node(&mut self) {
        self.current = self.t.next();
    }

    fn parse_number(&mut self) -> Option<Node> {
        let _n = match &self.current {
            Some(node) => node,
            None => return None,
        };

        None
    }

    fn parse_statement(&mut self) -> Option<Node> {
        let _n = match &self.current {
            Some(node) => node,
            None => return None,
        };

        None
    }

    pub fn parse_ast(&mut self) -> Program {
        let mut program = Program::new();

        // interface Program <: Node {
        //   type: "Program";
        //   body: [ Directive | Statement ];
        // }
        let body = Vec::new();
        program.set_body(body);

        program
    }
}
