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

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Node {
    /// https://github.com/estree/estree/blob/master/es5.md#expressionstatement
    ExpressionStatement(Option<Rc<Node>>),
    /// https://github.com/estree/estree/blob/master/es5.md#variabledeclaration
    VariableDeclaration(Vec<Option<Rc<Node>>>),
    /// https://github.com/estree/estree/blob/master/es5.md#variabledeclarator
    VariableDeclarator {
        id: Option<Rc<Node>>,
        init: Option<Rc<Node>>,
    },
    /// https://github.com/estree/estree/blob/master/es5.md#binaryexpression
    BinaryExpression {
        operator: char,
        left: Option<Rc<Node>>,
        right: Option<Rc<Node>>,
    },
    /// https://github.com/estree/estree/blob/master/es5.md#identifier
    /// https://262.ecma-international.org/12.0/#prod-Identifier
    Identifier(String),
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
    ) -> Option<Rc<Self>> {
        Some(Rc::new(Node::BinaryExpression {
            operator,
            left,
            right,
        }))
    }

    pub fn new_expression_statement(expression: Option<Rc<Node>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::ExpressionStatement(expression)))
    }

    pub fn new_variable_declarator(
        id: Option<Rc<Node>>,
        init: Option<Rc<Node>>,
    ) -> Option<Rc<Self>> {
        Some(Rc::new(Node::VariableDeclarator { id, init }))
    }

    pub fn new_variable_declaration(declarations: Vec<Option<Rc<Node>>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::VariableDeclaration(declarations)))
    }

    pub fn new_identifier(name: String) -> Option<Rc<Self>> {
        Some(Rc::new(Node::Identifier(name)))
    }

    pub fn new_numeric_literal(value: u64) -> Option<Rc<Self>> {
        Some(Rc::new(Node::NumericLiteral(value)))
    }

    pub fn new_string_literal(value: String) -> Option<Rc<Self>> {
        Some(Rc::new(Node::StringLiteral(value)))
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

    /// Literal ::= ( <DECIMAL_LITERAL> | <HEX_INTEGER_LITERAL> | <STRING_LITERAL> |
    ///               <BOOLEAN_LITERAL> | <NULL_LITERAL> | <REGULAR_EXPRESSION_LITERAL> )
    ///
    /// PrimaryExpression ::= "this"
    ///                     | ObjectLiteral
    ///                     | ( "(" Expression ")" )
    ///                     | Identifier
    ///                     | ArrayLiteral
    ///                     | Literal
    fn primary_expression(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("literal: token {:?}", t);

        match t {
            Token::Number(value) => Node::new_numeric_literal(value),
            Token::StringLiteral(value) => Node::new_string_literal(value),
            Token::Identifier(value) => Node::new_identifier(value),
            _ => unimplemented!("token {:?} is not supported", t),
        }
    }

    /// MemberExpression ::= ( ( FunctionExpression | PrimaryExpression ) ( MemberExpressionPart)* )
    ///                    | AllocationExpression
    /// LeftHandSideExpression ::= CallExpression
    ///                          | MemberExpression
    /// PostfixExpression ::= LeftHandSideExpression ( PostfixOperator )?
    /// UnaryExpression ::= ( PostfixExpression | ( UnaryOperator UnaryExpression )+ )
    /// MultiplicativeExpression ::= UnaryExpression ( MultiplicativeOperator UnaryExpression )*
    /// AdditiveExpression ::= MultiplicativeExpression ( AdditiveOperator MultiplicativeExpression )*
    /// ShiftExpression ::= AdditiveExpression ( ShiftOperator AdditiveExpression )*
    /// RelationalExpression ::= ShiftExpression ( RelationalOperator ShiftExpression )*
    /// EqualityExpression  ::= RelationalExpression ( EqualityOperator RelationalExpression )*
    /// BitwiseANDExpression ::= EqualityExpression ( BitwiseANDOperator EqualityExpression )*
    /// BitwiseXORExpression ::= BitwiseANDExpression ( BitwiseXOROperator BitwiseANDExpression )*
    /// BitwiseORExpression ::= BitwiseXORExpression ( BitwiseOROperator BitwiseXORExpression )*
    /// LogicalANDExpression ::= BitwiseORExpression ( LogicalANDOperator BitwiseORExpression )*
    /// LogicalORExpression ::= LogicalANDExpression ( LogicalOROperator LogicalANDExpression )*
    /// ConditionalExpression ::= LogicalORExpression ( "?" AssignmentExpression ":" AssignmentExpression )?
    /// AssignmentExpression ::= ( LeftHandSideExpression AssignmentOperator AssignmentExpression
    ///                          | ConditionalExpression )
    ///
    /// Expression ::= AssignmentExpression ( "," AssignmentExpression )*
    fn expression(&mut self) -> Option<Rc<Node>> {
        let left = self.primary_expression();

        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("expression: token {:?}", t);

        match t {
            Token::Punctuator(c) => match c {
                // AdditiveOperator
                '+' | '-' => Node::new_binary_expr(c, left, self.primary_expression()),
                ';' => left,
                _ => unimplemented!("`Punctuator` token with {} is not supported", c),
            },
            _ => None,
        }
    }

    /// Identifier ::= <IDENTIFIER_NAME>
    fn identifier(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("identifier: token {:?}", t);

        match t {
            Token::Identifier(name) => Node::new_identifier(name),
            _ => return None,
        }
    }

    /// Initialiser ::= "=" AssignmentExpression
    fn initialiser(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };

        println!("initialiser: token {:?}", t);

        match t {
            Token::Punctuator(c) => match c {
                '=' => self.expression(),
                _ => None,
            },
            _ => None,
        }
    }

    /// VariableDeclarationList ::= VariableDeclaration ( "," VariableDeclaration )*
    /// VariableDeclaration ::= Identifier ( Initialiser )?
    fn variable_declaration(&mut self) -> Option<Rc<Node>> {
        let ident = self.identifier();

        // TODO: support multiple declarator
        let declarator = Node::new_variable_declarator(ident, self.initialiser());

        let mut declarations = Vec::new();
        declarations.push(declarator);

        Node::new_variable_declaration(declarations)
    }

    /// https://262.ecma-international.org/12.0/#prod-Statement
    ///
    /// VariableStatement ::= "var" VariableDeclarationList ( ";" )?
    /// ExpressionStatement ::= Expression ( ";" )?
    ///
    /// Statement ::= ExpressionStatement | VariableStatement
    fn statement(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.peek() {
            Some(t) => t,
            None => return None,
        };

        println!("statement: {:?}", t);

        let node = match t {
            Token::Keyword(keyword) => {
                // consume "var".
                assert!(self.t.next().is_some());

                println!("statement: keyword {:?}", keyword);

                self.variable_declaration()
            }
            _ => Node::new_expression_statement(self.expression()),
        };

        if let Some(t) = self.t.peek() {
            if let Token::Punctuator(c) = t {
                // consume ';'
                if c == ';' {
                    self.t.next();
                }
            }
        }

        node
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
