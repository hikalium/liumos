//! This is a part of "3. Tokenizing and Parsing CSS" in the CSS Syntax Module Level 3 spec.
//! https://www.w3.org/TR/css-syntax-3/#tokenizing-and-parsing
//!
//! 5. Parsing
//! https://www.w3.org/TR/css-syntax-3/#tokenization

use crate::alloc::string::ToString;
use crate::renderer::css_token::*;
use alloc::string::String;
use alloc::vec::Vec;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Declaration {
    property: String,
    pub value: String,
}

impl Declaration {
    pub fn new() -> Self {
        Self {
            property: String::new(),
            value: String::new(),
        }
    }

    pub fn set_property(&mut self, property: String) {
        self.property = property;
    }

    pub fn set_value(&mut self, value: String) {
        self.value = value;
    }
}

#[allow(dead_code)]
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

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
/// https://www.w3.org/TR/css-syntax-3/#qualified-rule
/// https://www.w3.org/TR/css-syntax-3/#style-rules
pub struct QualifiedRule {
    // TODO: support multiple selectors
    pub selector: Selector,
    /// https://www.w3.org/TR/selectors-4/#typedef-selector-list
    /// The prelude of the qualified rule is parsed as a <selector-list>.
    selectors: Vec<Selector>,
    /// https://www.w3.org/TR/css-syntax-3/#parse-a-list-of-declarations
    /// The content of the qualified rule’s block is parsed as a list of declarations.
    pub declarations: Vec<Declaration>,
}

impl QualifiedRule {
    pub fn new() -> Self {
        Self {
            selector: Selector::TypeSelector("".to_string()),
            selectors: Vec::new(),
            declarations: Vec::new(),
        }
    }

    #[allow(dead_code)]
    pub fn set_selectors(&mut self, selectors: Vec<Selector>) {
        self.selectors = selectors;
    }

    pub fn set_selector(&mut self, selector: Selector) {
        self.selector = selector;
    }

    pub fn set_declarations(&mut self, declarations: Vec<Declaration>) {
        self.declarations = declarations;
    }
}

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

#[derive(Debug, Clone)]
pub struct CssParser {
    t: CssTokenizer,
    /// https://www.w3.org/TR/css-syntax-3/#reconsume-the-current-input-token
    /// True if the next time an algorithm instructs you to consume the next input token, instead
    /// do nothing (retain the current input token unchanged).
    reconsume: bool,
}

impl CssParser {
    pub fn new(t: CssTokenizer) -> Self {
        Self {
            t,
            reconsume: false,
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-component-value
    fn consume_component_value(&mut self, current_token: &CssToken) -> String {
        let next_token;
        let token = match self.reconsume {
            true => {
                self.reconsume = false;
                current_token
            }
            false => {
                next_token = match self.t.next() {
                    Some(t) => t,
                    None => return String::new(),
                };
                &next_token
            }
        };

        match token {
            // TODO: support nested style
            /*
            CssToken::OpenCurly => {
                // Consume a simple block and return it.
                return self.consume_qualified_rule(&token);
            }
            */
            CssToken::Ident(ident) => ident.to_string(),
            _ => {
                panic!("Parse error: {:?} is an unexpected token.", token);
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#qualified-rule
    /// Note: Most qualified rules will be style rules, where the prelude is a selector [SELECT]
    /// and the block a list of declarations.
    fn consume_selector(&mut self, current_token: &CssToken) -> Selector {
        let next_token;
        let token = match self.reconsume {
            true => {
                self.reconsume = false;
                current_token
            }
            false => {
                next_token = match self.t.next() {
                    Some(t) => t,
                    // TODO: return an error.
                    None => return Selector::TypeSelector("".to_string()),
                };
                &next_token
            }
        };

        match token {
            CssToken::HashToken(value) => Selector::IdSelector(value[1..].to_string()),
            CssToken::Delim(_delim) => Selector::ClassSelector(self.consume_component_value(token)),
            CssToken::Ident(ident) => Selector::TypeSelector(ident.to_string()),
            _ => {
                panic!("Parse error: {:?} is an unexpected token.", token);
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-a-declaration
    fn consume_declaration(&mut self, current_token: &CssToken) -> Option<Declaration> {
        let next_token;
        let token = match self.reconsume {
            true => {
                self.reconsume = false;
                current_token
            }
            false => {
                next_token = match self.t.next() {
                    Some(t) => t,
                    None => return None,
                };
                &next_token
            }
        };

        // Create a new declaration with its name set to the value of the current input token.
        let mut declaration = Declaration::new();
        self.reconsume = true;
        declaration.set_property(self.consume_component_value(&token));

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
        declaration.set_value(self.consume_component_value(&token));

        Some(declaration)
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-simple-block
    /// https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-declarations
    /// Note: Most qualified rules will be style rules, where the prelude is a selector [SELECT] and
    /// the block a list of declarations.
    fn consume_list_of_declarations(&mut self, current_token: &CssToken) -> Vec<Declaration> {
        let mut declarations = Vec::new();

        loop {
            let next_token;
            let token = match self.reconsume {
                true => {
                    self.reconsume = false;
                    current_token
                }
                false => {
                    next_token = match self.t.next() {
                        Some(t) => t,
                        None => return declarations,
                    };
                    &next_token
                }
            };

            match token {
                CssToken::CloseCurly => {
                    // https://www.w3.org/TR/css-syntax-3/#ending-token
                    return declarations;
                }
                CssToken::SemiColon => {
                    // Do nothing.
                }
                CssToken::Ident(_ident) => {
                    self.reconsume = true;
                    match self.consume_declaration(&token) {
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

    /*
    #[allow(dead_code)]
    /// https://www.w3.org/TR/css-syntax-3/#parse-list-of-component-values
    fn consume_list_of_component_values(&mut self, current_token: &CssToken) -> Vec<String> {
        let mut values = Vec::new();

        loop {
            let next_token;
            let token = match self.reconsume {
                true => {
                    self.reconsume = false;
                    current_token
                }
                false => {
                    next_token = match self.t.next() {
                        Some(t) => t,
                        None => return values,
                    };
                    &next_token
                }
            };

            values.push(self.consume_component_value(token));
        }
    }
    */

    /// https://www.w3.org/TR/css-syntax-3/#consume-qualified-rule
    /// https://www.w3.org/TR/css-syntax-3/#qualified-rule
    /// https://www.w3.org/TR/css-syntax-3/#style-rules
    fn consume_qualified_rule(&mut self, current_token: &CssToken) -> Option<QualifiedRule> {
        let mut rule = QualifiedRule::new();

        loop {
            let next_token;
            let token = match self.reconsume {
                true => {
                    self.reconsume = false;
                    current_token
                }
                false => {
                    next_token = match self.t.next() {
                        Some(t) => t,
                        None => return None,
                    };
                    &next_token
                }
            };

            match token {
                CssToken::OpenCurly => {
                    // Consume a simple block and assign it to the qualified rule’s block. Return
                    // the qualified rule.

                    // The content of the qualified rule’s block is parsed as a list of
                    // declarations.
                    rule.set_declarations(self.consume_list_of_declarations(&token));
                    return Some(rule);
                }
                _ => {
                    // Reconsume the current input token. Consume a component value. Append the
                    // returned value to the qualified rule’s prelude.

                    // The prelude of the qualified rule is parsed as a <selector-list>.
                    // https://www.w3.org/TR/css-syntax-3/#css-parse-something-according-to-a-css-grammar
                    self.reconsume = true;
                    // TODO: support multiple selectors
                    /*
                    let mut selectors = Vec::new();
                    selectors.push(Selector::TypeSelector(self.consume_component_value(&token)));
                    rule.set_selectors(selectors);
                    */
                    rule.set_selector(self.consume_selector(&token));
                }
            }
        }
    }

    /// https://www.w3.org/TR/css-syntax-3/#consume-a-list-of-rules
    fn consume_list_of_rules(&mut self) -> Vec<QualifiedRule> {
        let mut rules = Vec::new();

        let mut token;
        loop {
            token = match self.t.next() {
                Some(t) => t,
                None => return rules,
            };

            match token {
                _ => {
                    // Reconsume the current input token. Consume a qualified rule. If anything is
                    // returned, append it to the list of rules.
                    self.reconsume = true;
                    let rule = self.consume_qualified_rule(&token);
                    match rule {
                        Some(r) => rules.push(r),
                        None => {}
                    }
                }
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
