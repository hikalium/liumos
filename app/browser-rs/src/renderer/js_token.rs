//! https://262.ecma-international.org/12.0/#sec-ecmascript-language-lexical-grammar

use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum JsToken {
    /// https://262.ecma-international.org/12.0/#sec-identifier-names
    Identifier(String),
    /// https://262.ecma-international.org/12.0/#sec-keywords-and-reserved-words
    Keyword(String),
    /// https://262.ecma-international.org/12.0/#sec-punctuators
    Punctuator(char),
    /// https://262.ecma-international.org/12.0/#sec-literals-numeric-literals
    Number(u64),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct JsLexer {
    pos: usize,
    input: Vec<char>,
}

#[allow(dead_code)]
impl JsLexer {
    pub fn new(js: String) -> Self {
        Self {
            pos: 0,
            input: js.chars().collect(),
        }
    }

    fn consume_number(&mut self) -> u64 {
        let mut num = 0;
        let mut count = 0;

        loop {
            let c = self.input[self.pos];
            match c {
                '0'..='9' => {
                    num = num * count * 10 + (c.to_digit(10).unwrap() as u64);
                    count += 1;
                    self.pos += 1;
                }
                _ => break,
            }
        }

        return num;
    }
}

impl Iterator for JsLexer {
    type Item = JsToken;

    fn next(&mut self) -> Option<Self::Item> {
        if self.pos >= self.input.len() {
            return None;
        }

        let c = self.input[self.pos];

        let token = match c {
            '+' | '-' => JsToken::Punctuator(c),
            '0'..='9' => JsToken::Number(self.consume_number()),
            _ => unimplemented!("char {} is not supported yet", c),
        };

        self.pos += 1;
        Some(token)
    }
}
