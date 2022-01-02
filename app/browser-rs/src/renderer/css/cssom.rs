//! This is a part of "3. Tokenizing and Parsing CSS" in the "CSS Syntax Module Level 3" spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing
//!
//! 5. Parsing
//! https://www.w3.org/TR/css-syntax-3/#tokenization

use crate::alloc::string::ToString;
use crate::renderer::css::token::*;
use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

// e.g.
// div {
//   background-color: green;
//   width: 100;
// }
// p {
//   color: red;
// }
//
// StyleSheet
// |-- QualifiedRule
//     |-- Selector
//         |-- div
//     |-- Vec<Declaration>
//         |-- background-color: green
//         |-- width: 100
// |-- QualifiedRule
//     |-- Selector
//         |-- p
//     |-- Vec<Declaration>
//         |-- color: red

#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/cssom-1/#cssstylesheet
pub struct StyleSheet {
    /// https://drafts.csswg.org/cssom/#dom-cssstylesheet-cssrules
    pub rules: Vec<QualifiedRule>,
}

impl StyleSheet {
    pub fn new() -> Self {
        Self { rules: Vec::new() }
    }

    pub fn set_rules(&mut self, rules: Vec<QualifiedRule>) {
        self.rules = rules;
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/css-syntax-3/#qualified-rule
/// https://www.w3.org/TR/css-syntax-3/#style-rules
pub struct QualifiedRule {
    // TODO: support multiple selectors
    /// https://www.w3.org/TR/selectors-4/#typedef-selector-list
    /// The prelude of the qualified rule is parsed as a <selector-list>.
    pub selector: Selector,
    /// https://www.w3.org/TR/css-syntax-3/#parse-a-list-of-declarations
    /// The content of the qualified rule’s block is parsed as a list of declarations.
    pub declarations: Vec<Declaration>,
}

impl QualifiedRule {
    pub fn new() -> Self {
        Self {
            selector: Selector::TypeSelector("".to_string()),
            declarations: Vec::new(),
        }
    }

    pub fn set_selector(&mut self, selector: Selector) {
        self.selector = selector;
    }

    pub fn set_declarations(&mut self, declarations: Vec<Declaration>) {
        self.declarations = declarations;
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/selectors-3/#selectors
pub enum Selector {
    /// https://www.w3.org/TR/selectors-3/#type-selectors
    TypeSelector(String),
    /// https://www.w3.org/TR/selectors-3/#class-html
    ClassSelector(String),
    /// https://www.w3.org/TR/selectors-3/#id-selectors
    IdSelector(String),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Declaration {
    pub property: String,
    pub value: ComponentValue,
}

/// https://www.w3.org/TR/css-syntax-3/#declaration
impl Declaration {
    pub fn new() -> Self {
        Self {
            property: String::new(),
            value: ComponentValue::Keyword(String::new()),
        }
    }

    pub fn set_property(&mut self, property: String) {
        self.property = property;
    }

    pub fn set_value(&mut self, value: ComponentValue) {
        self.value = value;
    }
}

/// https://www.w3.org/TR/css-syntax-3/#component-value
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ComponentValue {
    /// https://www.w3.org/TR/css-values-3/#keywords
    Keyword(String),
    /// https://www.w3.org/TR/css-values-3/#numeric-types
    Number(u64),
}

#[derive(Debug, Clone)]
pub struct CssParser {
    t: CssTokenizer,
    /// https://www.w3.org/TR/css-syntax-3/#reconsume-the-current-input-token
    /// True if the next time an algorithm instructs you to consume the next input token, instead
    /// do nothing (retain the current input token unchanged).
    reconsumed_token: Option<CssToken>,
}

impl CssParser {
    pub fn new(t: CssTokenizer) -> Self {
        Self {
            t,
            reconsumed_token: None,
        }
    }

    fn consume_ident(&mut self) -> String {
        let token = match self.reconsumed_token.clone() {
            Some(t) => {
                self.reconsumed_token = None;
                t
            }
            None => match self.t.next() {
                Some(t) => t,
                None => unimplemented!("Parse error: should have next token but got None"),
            },
        };

        match token {
            CssToken::Ident(ident) => ident.to_string(),
            _ => {
                panic!("Parse error: {:?} is an unexpected token.", token);
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-component-value
    fn consume_component_value(&mut self) -> ComponentValue {
        let token = match self.reconsumed_token.clone() {
            Some(t) => {
                self.reconsumed_token = None;
                t
            }
            None => match self.t.next() {
                Some(t) => t,
                None => return ComponentValue::Keyword(String::new()),
            },
        };

        match token {
            CssToken::Ident(ident) => ComponentValue::Keyword(ident.to_string()),
            CssToken::Number(num) => ComponentValue::Number(num),
            _ => {
                panic!("Parse error: {:?} is an unexpected token.", token);
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#qualified-rule
    /// Note: Most qualified rules will be style rules, where the prelude is a selector [SELECT]
    /// and the block a list of declarations.
    fn consume_selector(&mut self) -> Selector {
        let token = match self.reconsumed_token.clone() {
            Some(t) => {
                self.reconsumed_token = None;
                t
            }
            None => {
                match self.t.next() {
                    Some(t) => t,
                    // TODO: return an error.
                    None => return Selector::TypeSelector(String::new()),
                }
            }
        };

        match token {
            CssToken::HashToken(value) => Selector::IdSelector(value[1..].to_string()),
            CssToken::Delim(_delim) => Selector::ClassSelector(self.consume_ident()),
            CssToken::Ident(ident) => Selector::TypeSelector(ident.to_string()),
            _ => {
                panic!("Parse error: {:?} is an unexpected token.", token);
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-a-declaration
    fn consume_declaration(&mut self) -> Option<Declaration> {
        let token = match self.reconsumed_token.clone() {
            Some(t) => {
                self.reconsumed_token = None;
                t
            }
            None => match self.t.next() {
                Some(t) => t,
                None => return None,
            },
        };

        // Create a new declaration with its name set to the value of the current input token.
        let mut declaration = Declaration::new();
        self.reconsumed_token = Some(token);
        declaration.set_property(self.consume_ident());

        // If the next input token is anything other than a <colon-token>, this is a parse error.
        // Return nothing. Otherwise, consume the next input token.
        match self.t.next() {
            Some(token) => match token {
                CssToken::Colon => {}
                _ => return None,
            },
            None => return None,
        }

        // As long as the next input token is anything other than an <EOF-token>, consume a
        // component value and append it to the declaration’s value.
        declaration.set_value(self.consume_component_value());

        Some(declaration)
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-simple-block
    /// https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-declarations
    /// Note: Most qualified rules will be style rules, where the prelude is a selector [SELECT] and
    /// the block a list of declarations.
    fn consume_list_of_declarations(&mut self) -> Vec<Declaration> {
        let mut declarations = Vec::new();

        loop {
            let token = match self.reconsumed_token.clone() {
                Some(t) => {
                    self.reconsumed_token = None;
                    t
                }
                None => match self.t.next() {
                    Some(t) => t,
                    None => return declarations,
                },
            };

            match token {
                CssToken::CloseCurly => {
                    // https://www.w3.org/TR/css-syntax-3/#ending-token
                    return declarations;
                }
                CssToken::SemiColon => {
                    // Do nothing.
                }
                CssToken::Ident(ref _ident) => {
                    self.reconsumed_token = Some(token);
                    match self.consume_declaration() {
                        Some(declaration) => declarations.push(declaration),
                        None => {}
                    }
                }
                _ => {
                    panic!("Parse error: {:?} is an unexpected token.", token);
                }
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-qualified-rule
    /// https://www.w3.org/TR/css-syntax-3/#qualified-rule
    /// https://www.w3.org/TR/css-syntax-3/#style-rules
    fn consume_qualified_rule(&mut self) -> Option<QualifiedRule> {
        let mut rule = QualifiedRule::new();

        loop {
            let token = match self.reconsumed_token.clone() {
                Some(t) => {
                    self.reconsumed_token = None;
                    t
                }
                None => {
                    match self.t.next() {
                        Some(t) => t,
                        // <EOF-token>
                        // "This is a parse error. Return nothing."
                        None => return None,
                    }
                }
            };

            match token {
                CssToken::OpenCurly => {
                    // "Consume a simple block and assign it to the qualified rule’s block. Return
                    // the qualified rule."

                    // The content of the qualified rule’s block is parsed as a list of
                    // declarations.
                    rule.set_declarations(self.consume_list_of_declarations());
                    return Some(rule);
                }
                _ => {
                    // "Reconsume the current input token. Consume a component value. Append the
                    // returned value to the qualified rule’s prelude."

                    // The prelude of the qualified rule is parsed as a <selector-list>.
                    // https://www.w3.org/TR/css-syntax-3/#css-parse-something-according-to-a-css-grammar
                    self.reconsumed_token = Some(token);
                    rule.set_selector(self.consume_selector());
                }
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-rules
    fn consume_list_of_rules(&mut self) -> Vec<QualifiedRule> {
        // "Create an initially empty list of rules."
        let mut rules = Vec::new();

        loop {
            let token = match self.t.next() {
                Some(t) => t,
                None => return rules,
            };

            // TODO: suppor other cases
            // https://www.w3.org/TR/css-syntax-3/#consume-list-of-rules

            // anything else
            // "Reconsume the current input token. Consume a qualified rule. If anything is
            // returned, append it to the list of rules."
            self.reconsumed_token = Some(token);
            let rule = self.consume_qualified_rule();
            match rule {
                Some(r) => rules.push(r),
                None => {}
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#parse-stylesheet
    pub fn parse_stylesheet(&mut self) -> StyleSheet {
        // 1. Create a new stylesheet.
        let mut sheet = StyleSheet::new();

        // 2. Consume a list of rules from the stream of tokens, with the top-level flag set. Let
        // the return value be rules.
        // 3. Assign rules to the stylesheet’s value.
        sheet.set_rules(self.consume_list_of_rules());

        // 4. Return the stylesheet.
        sheet
    }
}
