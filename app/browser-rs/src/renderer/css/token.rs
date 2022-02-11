//! This is a part of "3. Tokenizing and Parsing CSS" in the "CSS Syntax Module Level 3" spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing
//!
//! 4. Tokenization
//! https://www.w3.org/TR/css-syntax-3/#tokenization

use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/css-syntax-3/#consume-token
/// https://www.w3.org/TR/css-syntax-3/#tokenization
pub enum CssToken {
    /// https://www.w3.org/TR/css-syntax-3/#typedef-hash-token
    HashToken(String),
    /// https://www.w3.org/TR/css-syntax-3/#typedef-delim-token
    Delim(char),
    /// https://www.w3.org/TR/css-syntax-3/#typedef-number-token
    Number(u64),
    /// https://www.w3.org/TR/css-syntax-3/#typedef-colon-token
    Colon,
    /// https://www.w3.org/TR/css-syntax-3/#typedef-semicolon-token
    SemiColon,
    /// https://www.w3.org/TR/css-syntax-3/#tokendef-open-curly
    OpenCurly,
    /// https://www.w3.org/TR/css-syntax-3/#tokendef-close-curly
    CloseCurly,
    /// https://www.w3.org/TR/css-syntax-3/#typedef-ident-token
    Ident(String),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CssTokenizer {
    pos: usize,
    input: Vec<char>,
}

impl CssTokenizer {
    pub fn new(css: String) -> Self {
        Self {
            pos: 0,
            input: css.chars().collect(),
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-name
    fn consume_ident_token(&mut self) -> String {
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

        return s;
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-number
    /// https://www.w3.org/TR/css-syntax-3/#consume-a-numeric-token
    fn consume_numeric_token(&mut self) -> u64 {
        let mut num = 0;

        loop {
            if self.pos >= self.input.len() {
                return num;
            }

            let c = self.input[self.pos];

            match c {
                '0'..='9' => {
                    num = num * 10 + (c.to_digit(10).unwrap() as u64);
                    self.pos += 1;
                }
                _ => break,
            }
        }

        return num;
    }
}

impl Iterator for CssTokenizer {
    type Item = CssToken;

    /// https://www.w3.org/TR/css-syntax-3/#consume-token
    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if self.pos >= self.input.len() {
                return None;
            }

            let c = self.input[self.pos];

            let token = match c {
                '#' => {
                    // TODO: support the case if the next token is not ident code point
                    // "Otherwise, return a <delim-token> with its value set to the current input
                    // code point."
                    // https://www.w3.org/TR/css-syntax-3/#consume-token
                    let value = self.consume_ident_token();
                    self.pos -= 1;
                    CssToken::HashToken(value)
                }
                '.' => CssToken::Delim('.'),
                ':' => CssToken::Colon,
                ';' => CssToken::SemiColon,
                '{' => CssToken::OpenCurly,
                '}' => CssToken::CloseCurly,
                // digit
                // Reconsume the current input code point, consume a numeric token, and return it.
                '1'..='9' => {
                    let t = CssToken::Number(self.consume_numeric_token());
                    self.pos -= 1;
                    t
                }
                // ident-start code point
                // Reconsume the current input code point, consume an ident-like token, and return
                // it.
                'a'..='z' | 'A'..='Z' | '_' => {
                    let t = CssToken::Ident(self.consume_ident_token());
                    self.pos -= 1;
                    t
                }
                // TODO: handle white spaces property
                // "Consume as much whitespace as possible. Return a <whitespace-token>."
                // https://www.w3.org/TR/css-syntax-3/#consume-token
                ' ' | '\n' => {
                    self.pos += 1;
                    continue;
                }
                _ => {
                    unimplemented!("char {} is not supported yet", c);
                }
            };

            self.pos += 1;
            return Some(token);
        }
    }
}
