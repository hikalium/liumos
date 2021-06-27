//! This is a part of "13.2.5 Tokenization" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tokenization

use alloc::string::String;
use alloc::vec::Vec;
use core::iter::Iterator;
#[allow(unused_imports)]
use liumlib::*;

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
pub enum Token {
    Doctype,
    StartTag { tag: String, self_closing: bool },
    EndTag { tag: String, self_closing: bool },
    Char(char),
    Eof,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Tokenizer {
    state: State,
    pos: usize,
    input: Vec<char>,
}

impl Tokenizer {
    pub fn new(html: String) -> Self {
        Self {
            state: State::Data,
            pos: 0,
            input: html.chars().collect(),
        }
    }

    fn switch_to(&mut self, s: State) {
        self.state = s;
    }

    /// Consume the next input character.
    fn consume_next_input(&mut self) -> char {
        let c = self.input[self.pos];
        self.pos += 1;
        c
    }
}

impl Iterator for Tokenizer {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
        if self.pos >= self.input.len() {
            return None;
        }

        loop {
            let c = self.consume_next_input();

            match self.state {
                State::Data => {
                    // https://html.spec.whatwg.org/multipage/parsing.html#data-state
                    if c == '<' {
                        self.switch_to(State::TagOpen);
                        continue;
                    }

                    if self.pos > self.input.len() {
                        return Some(Token::Eof);
                    }

                    return Some(Token::Char(c));
                }
                State::TagOpen => {
                    // https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
                }
                _ => {}
            }

            if self.pos == self.input.len() {
                return Some(Token::Eof);
            }
        }
    }
}
