//! This is a part of "3. Tokenizing and Parsing CSS" in the CSS Syntax Module Level 3 spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing

use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/css-syntax-3/#consume-token
pub enum Token {
    /// https://www.w3.org/TR/css-syntax-3/#typedef-colon-token
    Colon,
    SemiColon,
    /// https://www.w3.org/TR/css-syntax-3/#tokendef-open-curly
    OpenCurly,
    /// https://www.w3.org/TR/css-syntax-3/#tokendef-close-curly
    CloseCurly,
    /// https://www.w3.org/TR/css-syntax-3/#typedef-ident-token
    Ident(String),
    /// https://www.w3.org/TR/css-syntax-3/#typedef-eof-token
    Eof,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CssTokenizer {
    pos: usize,
    input: Vec<char>,
}

impl CssTokenizer {
    #[allow(dead_code)]
    pub fn new(css: String) -> Self {
        Self {
            pos: 0,
            input: css.chars().collect(),
        }
    }

    #[allow(dead_code)]
    /// https://www.w3.org/TR/css-syntax-3/#consume-name
    fn consume_name(&mut self) -> Token {
        let mut s = String::new();
        s.push(self.input[self.pos]);

        loop {
            self.pos += 1;
            let c = self.input[self.pos];
            match c {
                'a'..='z' | 'A'..='Z' | '0'..='9' | '-' | '_' => {
                    s.push(c);
                }
                _ => break,
            }
        }

        return Token::Ident(s);
    }
}

impl Iterator for CssTokenizer {
    type Item = Token;

    #[allow(dead_code)]
    fn next(&mut self) -> Option<Self::Item> {
        if self.pos >= self.input.len() {
            return None;
        }

        let c = self.input[self.pos];

        let token = match c {
            ':' => Token::Colon,
            ';' => Token::SemiColon,
            '{' => Token::OpenCurly,
            '}' => Token::CloseCurly,
            'a'..='z' | 'A'..='Z' => {
                let t = self.consume_name();
                self.pos -= 1;
                t
            }
            _ => {
                unimplemented!("char {} is not supported yet", c);
            }
        };

        self.pos += 1;
        Some(token)
    }
}
#[derive(Debug, Clone)]
pub struct Rule {
    selector: Token,  // h1
    pub key: Token,   // background-color
    pub value: Token, // red
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct CssParser {
    t: CssTokenizer,
}

#[allow(dead_code)]
impl CssParser {
    pub fn new(t: CssTokenizer) -> Self {
        Self { t }
    }

    pub fn construct_tree(&mut self) -> Vec<Rule> {
        let mut token = self.t.next();
        let mut rules = Vec::new();

        println!("token {:?}", token);
        // h1
        let selector = token.expect("expect selector ident");
        token = self.t.next();

        // {
        assert_eq!(Some(Token::OpenCurly), token);
        token = self.t.next();

        // background-color
        let key = token.expect("expect key ident");
        token = self.t.next();

        // :
        assert_eq!(Some(Token::Colon), token);
        token = self.t.next();

        // red
        let value = token.expect("expect value ident");
        token = self.t.next();

        // ;
        assert_eq!(Some(Token::SemiColon), token);
        token = self.t.next();

        // }
        assert_eq!(Some(Token::CloseCurly), token);
        token = self.t.next();

        rules.push(Rule {
            selector,
            key,
            value,
        });

        rules
    }
}
