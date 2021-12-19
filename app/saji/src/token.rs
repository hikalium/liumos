//! https://262.ecma-international.org/12.0/#sec-ecmascript-language-lexical-grammar

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

static RESERVED_WORDS: [&str; 1] = ["var"];

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Token {
    /// https://262.ecma-international.org/12.0/#sec-identifier-names
    Identifier(String),
    /// https://262.ecma-international.org/12.0/#sec-keywords-and-reserved-words
    Keyword(String),
    /// https://262.ecma-international.org/12.0/#sec-punctuators
    Punctuator(char),
    /// https://262.ecma-international.org/12.0/#sec-literals-string-literals
    StringLiteral(String),
    /// https://262.ecma-international.org/12.0/#sec-literals-numeric-literals
    Number(u64),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Lexer {
    pos: usize,
    input: Vec<char>,
}

#[allow(dead_code)]
impl Lexer {
    pub fn new(js: String) -> Self {
        Self {
            pos: 0,
            input: js.chars().collect(),
        }
    }

    fn consume_number(&mut self) -> u64 {
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

    fn consume_string(&mut self) -> String {
        let mut result = String::new();
        self.pos += 1;

        loop {
            if self.pos >= self.input.len() {
                return result;
            }

            if self.input[self.pos] == '"' {
                self.pos += 1;
                return result;
            }

            result.push(self.input[self.pos]);
            self.pos += 1;
        }
    }

    fn consume_identifier(&mut self) -> String {
        let mut result = String::new();

        loop {
            if self.pos >= self.input.len() {
                return result;
            }

            // https://262.ecma-international.org/12.0/#prod-IdentifierPart
            if self.input[self.pos].is_ascii_alphanumeric() || self.input[self.pos] == '$' {
                result.push(self.input[self.pos]);
                self.pos += 1;
            } else {
                return result;
            }
        }
    }

    fn check_reserved_word(&self) -> Option<String> {
        for word in RESERVED_WORDS {
            for i in 0..word.len() {
                if word.chars().nth(i).expect("failed to access to i-th char")
                    != self.input[self.pos + i]
                {
                    return None;
                }
            }
            return Some(word.to_string());
        }

        None
    }

    pub fn peek(&mut self) -> Option<Token> {
        let start_position = self.pos;

        let token = self.get_next_token();

        // Restore the start position to avoid consuming input.
        self.pos = start_position;

        token
    }

    fn get_next_token(&mut self) -> Option<Token> {
        if self.pos >= self.input.len() {
            return None;
        }

        match self.check_reserved_word() {
            Some(keyword) => {
                self.pos += keyword.len();
                let token = Some(Token::Keyword(keyword));
                return token;
            }
            None => {}
        }

        let mut c = self.input[self.pos];

        // skip a white space
        while c == ' ' {
            self.pos += 1;
            c = self.input[self.pos];
        }

        let token = match c {
            '+' | '-' | ';' | '=' => {
                let t = Token::Punctuator(c);
                self.pos += 1;
                t
            }
            '"' => Token::StringLiteral(self.consume_string()),
            '0'..='9' => Token::Number(self.consume_number()),
            // https://262.ecma-international.org/12.0/#prod-IdentifierStart
            'a'..='z' | 'A'..='Z' | '_' | '$' => Token::Identifier(self.consume_identifier()),
            _ => unimplemented!("char {} is not supported yet", c),
        };
        Some(token)
    }
}

impl Iterator for Lexer {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
        self.get_next_token()
    }
}
