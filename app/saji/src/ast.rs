//! https://github.com/estree/estree
//! https://astexplorer.net/

use crate::alloc::string::ToString;
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
    /// https://github.com/estree/estree/blob/master/es5.md#blockstatement
    BlockStatement { body: Vec<Option<Rc<Node>>> },
    /// https://github.com/estree/estree/blob/master/es5.md#returnstatement
    ReturnStatement { argument: Option<Rc<Node>> },
    /// https://github.com/estree/estree/blob/master/es5.md#functions
    /// https://github.com/estree/estree/blob/master/es5.md#functiondeclaration
    FunctionDeclaration {
        id: Option<Rc<Node>>,
        params: Vec<Option<Rc<Node>>>,
        body: Option<Rc<Node>>,
    },
    /// https://github.com/estree/estree/blob/master/es5.md#variabledeclaration
    VariableDeclaration { declarations: Vec<Option<Rc<Node>>> },
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
    /// https://github.com/estree/estree/blob/master/es5.md#callexpression
    CallExpression {
        callee: Option<Rc<Node>>,
        arguments: Vec<Option<Rc<Node>>>,
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

    pub fn new_expression_statement(expression: Option<Rc<Self>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::ExpressionStatement(expression)))
    }

    pub fn new_block_statement(body: Vec<Option<Rc<Self>>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::BlockStatement { body }))
    }

    pub fn new_return_statement(argument: Option<Rc<Self>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::ReturnStatement { argument }))
    }

    pub fn new_function_declaration(
        id: Option<Rc<Self>>,
        params: Vec<Option<Rc<Self>>>,
        body: Option<Rc<Self>>,
    ) -> Option<Rc<Self>> {
        Some(Rc::new(Node::FunctionDeclaration { id, params, body }))
    }

    pub fn new_variable_declarator(
        id: Option<Rc<Self>>,
        init: Option<Rc<Self>>,
    ) -> Option<Rc<Self>> {
        Some(Rc::new(Node::VariableDeclarator { id, init }))
    }

    pub fn new_variable_declaration(declarations: Vec<Option<Rc<Self>>>) -> Option<Rc<Self>> {
        Some(Rc::new(Node::VariableDeclaration { declarations }))
    }

    pub fn new_call_expression(
        callee: Option<Rc<Self>>,
        arguments: Vec<Option<Rc<Self>>>,
    ) -> Option<Rc<Self>> {
        Some(Rc::new(Node::CallExpression { callee, arguments }))
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
    ///
    /// MemberExpression ::= ( ( FunctionExpression | PrimaryExpression ) ( MemberExpressionPart)* )
    ///                    | AllocationExpression
    /// CallExpression ::= MemberExpression Arguments ( CallExpressionPart )*
    ///
    /// LeftHandSideExpression ::= CallExpression | MemberExpression
    fn left_hand_side_expression(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("left_hand_side_expression: token {:?}", t);

        match t {
            // member expression
            Token::Number(value) => Node::new_numeric_literal(value),
            Token::StringLiteral(value) => Node::new_string_literal(value),
            Token::Identifier(value) => {
                let ident = Node::new_identifier(value);

                // call expression
                let t = match self.t.peek() {
                    Some(token) => token,
                    None => return None,
                };
                match t {
                    Token::Punctuator(c) => {
                        if c == '(' {
                            // consume '('
                            assert!(self.t.next().is_some());
                            let node = Node::new_call_expression(ident, Vec::new());

                            // TODO: support arguments

                            // consume ')'
                            assert!(self.t.next().is_some());
                            return node;
                        }
                    }
                    _ => {}
                }

                ident
            }
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
        let left = self.left_hand_side_expression();

        let t = match self.t.next() {
            Some(token) => token,
            None => return None,
        };
        println!("expression: token {:?}", t);

        match t {
            Token::Punctuator(c) => match c {
                // AdditiveOperator
                '+' | '-' => Node::new_binary_expr(c, left, self.left_hand_side_expression()),
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
    /// ReturnStatement ::= "return" ( Expression )? ( ";" )?
    ///
    /// Statement ::= ExpressionStatement | VariableStatement | ReturnStatement
    fn statement(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.peek() {
            Some(t) => t,
            None => return None,
        };

        println!("statement: {:?}", t);

        let node = match t {
            Token::Keyword(keyword) => {
                if keyword == "var" {
                    // consume "var"
                    assert!(self.t.next().is_some());

                    self.variable_declaration()
                } else if keyword == "return" {
                    // consume "return"
                    assert!(self.t.next().is_some());

                    Node::new_return_statement(self.expression())
                } else {
                    None
                }
            }
            _ => Node::new_expression_statement(self.expression()),
        };

        if let Some(t) = self.t.peek() {
            if let Token::Punctuator(c) = t {
                // consume ';'
                if c == ';' {
                    assert!(self.t.next().is_some());
                }
            }
        }

        node
    }

    /// FunctionBody ::= "{" ( SourceElements )? "}"
    fn function_body(&mut self) -> Option<Rc<Node>> {
        println!("function_body");
        // consume '{'
        match self.t.next() {
            Some(t) => match t {
                Token::Punctuator(c) => assert!(c == '{'),
                _ => unimplemented!("function should have open curly blacket but got {:?}", t),
            },
            None => unimplemented!("function should have open curly blacket but got None"),
        }

        let mut body = Vec::new();
        loop {
            // loop until hits '}'
            match self.t.peek() {
                Some(t) => match t {
                    Token::Punctuator(c) => {
                        if c == '}' {
                            // consume '}'
                            assert!(self.t.next().is_some());
                            return Node::new_block_statement(body);
                        }
                    }
                    _ => {}
                },
                None => {}
            }

            body.push(self.source_element());
        }
    }

    /// FormalParameterList ::= Identifier ( "," Identifier )*
    fn parameter_list(&mut self) -> Vec<Option<Rc<Node>>> {
        println!("parameter list");

        let mut params = Vec::new();

        // consume '('
        match self.t.next() {
            Some(t) => match t {
                Token::Punctuator(c) => assert!(c == '('),
                _ => unimplemented!("function should have `(` but got {:?}", t),
            },
            None => unimplemented!("function should have `(` but got None"),
        }

        loop {
            // push identifier to `params` until hits ')'
            match self.t.peek() {
                Some(t) => match t {
                    Token::Punctuator(c) => {
                        if c == ')' {
                            // consume ')'
                            assert!(self.t.next().is_some());
                            return params;
                        }
                        if c == ',' {
                            // consume ','
                            assert!(self.t.next().is_some());
                        }
                    }
                    _ => {
                        params.push(self.identifier());
                    }
                },
                None => return params,
            }
        }
    }

    /// FunctionDeclaration ::= "function" Identifier ( "(" ( FormalParameterList )? ")" ) FunctionBody
    fn function_declaration(&mut self) -> Option<Rc<Node>> {
        println!("function declaration");
        let id = self.identifier();
        let params = self.parameter_list();
        Node::new_function_declaration(id, params, self.function_body())
    }

    /// SourceElement ::= FunctionDeclaration | Statement
    fn source_element(&mut self) -> Option<Rc<Node>> {
        let t = match self.t.peek() {
            Some(t) => t,
            None => return None,
        };

        match t {
            Token::Keyword(keyword) => {
                if keyword == "function".to_string() {
                    // consume "function"
                    assert!(self.t.next().is_some());
                    self.function_declaration()
                } else {
                    self.statement()
                }
            }
            _ => self.statement(),
        }
    }

    /// SourceElements ::= ( SourceElement )+
    ///
    /// Program ::= ( SourceElements )? <EOF>
    pub fn parse_ast(&mut self) -> Program {
        let mut program = Program::new();

        // interface Program <: Node {
        //   type: "Program";
        //   body: [ Directive | Statement ];
        // }
        let mut body = Vec::new();

        loop {
            let node = self.source_element();

            match node {
                Some(n) => body.push(n),
                None => {
                    program.set_body(body);
                    return program;
                }
            }
        }
    }
}
