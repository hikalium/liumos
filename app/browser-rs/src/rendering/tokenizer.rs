//! This is a part of "13.2.5 Tokenization" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tokenization

use alloc::string::String;
use alloc::vec::Vec;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum State {
    Data,
    TagOpen,
    EndTagOpen,
    TagName,
    SelfClosingStartTag,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TokenType {
    Doctype,
    StartTag,
    EndTag,
    Char,
    Eof,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Token {
    token_type: TokenType,
    tag: String,
    self_closing: bool,
    data: String,
}

impl Token {
    pub fn new(token_type: TokenType, tag: String, self_closing: bool, data: String) -> Self {
        Self {
            token_type,
            tag,
            self_closing,
            data,
        }
    }
}

#[derive(Debug)]
pub struct Tokenizer {
    state: State,
    pos: usize,
    length: usize,
    html: String,
    tokens: Vec<Token>,
}

impl Tokenizer {
    pub fn new(html: String) -> Self {
        Self {
            state: State::Data,
            pos: 0,
            length: html.len(),
            html,
            tokens: Vec::new(),
        }
    }

    pub fn tokens(&mut self) -> Vec<Token> {
        loop {
            // https://html.spec.whatwg.org/multipage/parsing.html#preprocessing-the-input-stream

            match self.state {
                State::Data => {
                    if self.html[self.pos..].starts_with("<") {
                        self.state = State::TagOpen;
                        continue;
                    }
                    if self.pos == self.length {
                        self.append_eof();
                        return self.tokens.clone();
                    }
                    self.append_char();
                    break;
                }
                _ => {}
            }

            self.pos += 1;
        }

        self.tokens.clone()
    }

    pub fn append_eof(&mut self) {
        self.tokens.push(Token::new(
            TokenType::Eof,
            String::new(),
            false,
            String::new(),
        ));
    }

    fn append_char(&mut self) {
        self.tokens.push(Token::new(
            TokenType::Char,
            String::new(),
            false,
            String::new(),
        ));
    }
}
