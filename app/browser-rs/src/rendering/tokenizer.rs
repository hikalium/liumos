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
    latest_token: Option<Token>,
    input: Vec<char>,
}

impl Tokenizer {
    pub fn new(html: String) -> Self {
        Self {
            state: State::Data,
            pos: 0,
            reconsume: false,
            latest_token: None,
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
        assert!(self.latest_token.is_none());

        if start_tag_token {
            self.latest_token = Some(Token::StartTag {
                tag: String::new(),
                self_closing: false,
            });
        } else {
            self.latest_token = Some(Token::EndTag {
                tag: String::new(),
                self_closing: false,
            });
        }
    }

    /// Appends a char to the latest created Token `latest_token`.
    fn append_tag_name(&mut self, c: char) {
        assert!(self.latest_token.is_some());

        if let Some(t) = self.latest_token.as_mut() {
            match t {
                Token::StartTag {
                    ref mut tag,
                    self_closing: _,
                }
                | Token::EndTag {
                    ref mut tag,
                    self_closing: _,
                } => tag.push(c),
                _ => panic!("`latest_token` should be either StartTag or EndTag"),
            }
        }
    }

    /// Sets `self_closing` flag to the `latest_token`.
    fn set_self_closing_flag(&mut self) {
        assert!(self.latest_token.is_some());

        if let Some(t) = self.latest_token.as_mut() {
            match t {
                Token::StartTag {
                    tag: _,
                    ref mut self_closing,
                }
                | Token::EndTag {
                    tag: _,
                    ref mut self_closing,
                } => *self_closing = true,
                _ => panic!("`latest_token` should be either StartTag or EndTag"),
            }
        }
    }

    /// Returns `latest_token` and makes it to None.
    fn take_latest_token(&mut self) -> Option<Token> {
        assert!(self.latest_token.is_some());

        let t = self.latest_token.as_ref().and_then(|t| Some(t.clone()));
        self.latest_token = None;
        assert!(self.latest_token.is_none());

        t
    }

    /// Returns true if the current position is larger than the length of input.
    fn is_eof(&self) -> bool {
        self.pos > self.input.len()
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

                    if self.is_eof() {
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

                    if self.is_eof() {
                        return Some(Token::Eof);
                    }

                    self.reconsume = true;
                    self.state = State::Data;
                }
                // https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
                State::EndTagOpen => {
                    if self.is_eof() {
                        // invalid parse error.
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
                        return self.take_latest_token();
                    }

                    if c == '/' {
                        self.state = State::SelfClosingStartTag;
                        continue;
                    }

                    if c.is_ascii_uppercase() {
                        self.append_tag_name(c.to_ascii_lowercase());
                        continue;
                    }

                    if self.is_eof() {
                        // invalid parse error.
                        return Some(Token::Eof);
                    }

                    self.append_tag_name(c);
                }
                State::SelfClosingStartTag => {
                    if c == '>' {
                        self.set_self_closing_flag();
                        self.state = State::Data;
                        return self.take_latest_token();
                    }

                    if self.is_eof() {
                        // invalid parse error.
                        return Some(Token::Eof);
                    }
                }
            }
        }
    }
}
