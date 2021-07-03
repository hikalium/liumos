//! This is a part of "13.2.5 Tokenization" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tokenization

use alloc::string::String;
use alloc::vec::Vec;
use core::assert;
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
    reconsume: bool,
    new_token: Option<Token>,
    input: Vec<char>,
}

impl Tokenizer {
    pub fn new(html: String) -> Self {
        Self {
            state: State::Data,
            pos: 0,
            reconsume: false,
            new_token: None,
            input: html.chars().collect(),
        }
    }

    /// Consumes a next input character.
    fn consume_next_input(&mut self) -> char {
        //println!("{:?}", self);
        let c = self.input[self.pos];
        self.pos += 1;
        c
    }

    /// Reconsumes the character consumed in a previous step. The `reconsume` flag is reset once
    /// `reconsume_input` is called.
    fn reconsume_input(&mut self) -> char {
        self.reconsume = false;
        self.input[self.pos - 1]
    }

    /// Creates a StartTag or EndTag token.
    fn create_tag_open(&mut self, start_tag_token: bool) {
        assert!(self.new_token.is_none());

        if start_tag_token {
            self.new_token = Some(Token::StartTag {
                tag: String::new(),
                self_closing: false,
            });
        } else {
            self.new_token = Some(Token::EndTag {
                tag: String::new(),
                self_closing: false,
            });
        }
    }

    fn append_tag_name(&mut self, c: char) {
        if let Some(t) = self.new_token.as_mut() {
            match t {
                Token::StartTag {
                    ref mut tag,
                    self_closing: _,
                }
                | Token::EndTag {
                    ref mut tag,
                    self_closing: _,
                } => tag.push(c),
                _ => panic!("`new_token` should be either StartTag or EndTag"),
            }
        } else {
            panic!("`new_token` should have a value");
        }
    }
}

impl Iterator for Tokenizer {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
        if self.pos >= self.input.len() {
            return None;
        }

        loop {
            let c = match self.reconsume {
                true => self.reconsume_input(),
                false => self.consume_next_input(),
            };

            match self.state {
                // https://html.spec.whatwg.org/multipage/parsing.html#data-state
                State::Data => {
                    if c == '<' {
                        self.state = State::TagOpen;
                        continue;
                    }

                    if self.pos > self.input.len() {
                        return Some(Token::Eof);
                    }

                    return Some(Token::Char(c));
                }
                // https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
                State::TagOpen => {
                    if c == '/' {
                        self.state = State::EndTagOpen;
                        continue;
                    }

                    if c.is_ascii_alphabetic() {
                        self.reconsume = true;
                        self.state = State::TagName;
                        self.create_tag_open(true);
                        continue;
                    }

                    if self.pos > self.input.len() {
                        return Some(Token::Eof);
                    }

                    self.reconsume = true;
                    self.state = State::Data;
                }
                // https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
                State::EndTagOpen => {
                    if self.pos > self.input.len() {
                        // invalid
                        return Some(Token::Eof);
                    }

                    if c.is_ascii_alphabetic() {
                        self.reconsume = true;
                        self.state = State::TagName;
                        self.create_tag_open(false);
                        continue;
                    }
                }
                // https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
                State::TagName => {
                    if c == '>' {
                        self.state = State::Data;
                        let t = self.new_token.as_ref().and_then(|t| Some(t.clone()));
                        self.new_token = None;
                        return t;
                    }

                    if c.is_ascii_uppercase() {
                        self.append_tag_name(c.to_ascii_lowercase());
                        continue;
                    }

                    if self.pos > self.input.len() {
                        // invalid
                        return Some(Token::Eof);
                    }

                    self.append_tag_name(c);
                }
                _ => {}
            }
        }
    }
}
