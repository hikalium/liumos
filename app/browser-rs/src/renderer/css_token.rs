//! This is a part of "3. Tokenizing and Parsing CSS" in the CSS Syntax Module Level 3 spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing
//!
//! 4. Tokenization
//! https://www.w3.org/TR/css-syntax-3/#tokenization

use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/css-syntax-3/#consume-token
pub enum CssToken {
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
    fn consume_name(&mut self) -> CssToken {
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

        return CssToken::Ident(s);
    }
}

impl Iterator for CssTokenizer {
    type Item = CssToken;

    #[allow(dead_code)]
    fn next(&mut self) -> Option<Self::Item> {
        if self.pos >= self.input.len() {
            return None;
        }

        let c = self.input[self.pos];

        let token = match c {
            ':' => CssToken::Colon,
            ';' => CssToken::SemiColon,
            '{' => CssToken::OpenCurly,
            '}' => CssToken::CloseCurly,
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
