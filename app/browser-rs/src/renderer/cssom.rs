//! This is a part of "3. Tokenizing and Parsing CSS" in the CSS Syntax Module Level 3 spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing
//!
//! 5. Parsing
//! https://www.w3.org/TR/css-syntax-3/#tokenization

use crate::renderer::css_token::*;
use alloc::vec::Vec;
use liumlib::*;

#[derive(Debug, Clone)]
pub struct Rule {
    selector: CssToken,  // h1
    pub key: CssToken,   // background-color
    pub value: CssToken, // red
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
        assert_eq!(Some(CssToken::OpenCurly), token);
        token = self.t.next();

        // background-color
        let key = token.expect("expect key ident");
        token = self.t.next();

        // :
        assert_eq!(Some(CssToken::Colon), token);
        token = self.t.next();

        // red
        let value = token.expect("expect value ident");
        token = self.t.next();

        // ;
        assert_eq!(Some(CssToken::SemiColon), token);
        token = self.t.next();

        // }
        assert_eq!(Some(CssToken::CloseCurly), token);
        token = self.t.next();

        rules.push(Rule {
            selector,
            key,
            value,
        });

        assert!(token.is_none());

        rules
    }
}
