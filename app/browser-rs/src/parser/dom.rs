//! This is a part of "13.2.6 Tree construction" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

use crate::parser::tokenizer::*;
#[allow(unused_imports)]
use liumlib::*;

use alloc::prelude::v1::Box;
#[allow(unused_imports)]
use alloc::string::String;
#[allow(unused_imports)]
use alloc::vec::Vec;
#[allow(unused_imports)]
use core::assert;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
/// https://dom.spec.whatwg.org/#interface-node
pub struct Node {
    kind: NodeKind,
    pub parent: Option<Box<Node>>,
    pub first_child: Option<Box<Node>>,
    pub last_child: Option<Box<Node>>,
    pub previous_sibling: Option<Box<Node>>,
    pub next_sibling: Option<Box<Node>>,
}

#[allow(dead_code)]
///dom.spec.whatwg.org/#interface-node
impl Node {
    pub fn new(kind: NodeKind) -> Self {
        Self {
            kind,
            parent: None,
            first_child: None,
            last_child: None,
            previous_sibling: None,
            next_sibling: None,
        }
    }

    pub fn first_child(&self) -> Option<&Node> {
        self.first_child.as_ref().map(|n| n.as_ref())
    }

    pub fn last_child(&self) -> Option<&Node> {
        self.last_child.as_ref().map(|n| n.as_ref())
    }

    pub fn previous_sibling(&self) -> Option<&Node> {
        self.previous_sibling.as_ref().map(|n| n.as_ref())
    }

    pub fn next_sibling(&self) -> Option<&Node> {
        self.next_sibling.as_ref().map(|n| n.as_ref())
    }
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum NodeKind {
    /// https://dom.spec.whatwg.org/#interface-document
    Document,
    /// https://dom.spec.whatwg.org/#interface-element
    Element(Element),
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
/// https://dom.spec.whatwg.org/#interface-element
pub enum Element {
    /// https://html.spec.whatwg.org/multipage/dom.html#htmlelement
    HtmlElement(HtmlElementImpl),
}

/// https://html.spec.whatwg.org/multipage/semantics.html#htmlhtmlelement
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct HtmlElementImpl {}

impl HtmlElementImpl {
    pub fn new() -> Self {
        Self {}
    }
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum InsertionMode {
    Initial,
    BeforeHtml,
    BeforeHead,
    InHead,
    AfterHead,
    InBody,
    AfterBody,
    AfterAfterBody,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Parser<'a> {
    mode: InsertionMode,
    t: Tokenizer,
    /// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
    stack_of_open_elements: Vec<&'a Node>,
}

impl Parser<'_> {
    #[allow(dead_code)]
    pub fn new(t: Tokenizer) -> Self {
        Self {
            mode: InsertionMode::Initial,
            t,
            stack_of_open_elements: Vec::new(),
        }
    }

    /// Create an element based on the `tag` string.
    fn create_element_by_tag(&self, tag: &str) -> Node {
        if tag == "html" {
            return Node::new(NodeKind::Element(Element::HtmlElement(
                HtmlElementImpl::new(),
            )));
        }
        panic!("not supported this tag name: {}", tag);
    }

    /// Create an element for the token and append it to the Document object. Put this element in
    /// the stack of open elements.
    fn append_to_root(&mut self, root: &mut Box<Node>, tag: &str) {
        let node = Box::new(self.create_element_by_tag(tag));
        //node.parent = Some(root);
        //if root.first_child().is_none() {
        //root.first_child = Some(node);
        //}
        root.last_child = Some(node);

        //self.stack_of_open_elements.push(&node);
    }

    #[allow(dead_code)]
    pub fn construct_tree(&mut self) -> Box<Node> {
        let mut root = Box::new(Node::new(NodeKind::Document));

        let mut token = self.t.next();

        while token.is_some() {
            match self.mode {
                // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
                InsertionMode::Initial => self.mode = InsertionMode::BeforeHtml,

                // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
                InsertionMode::BeforeHtml => {
                    match token {
                        Some(Token::Doctype) => {
                            token = self.t.next();
                            continue;
                        }
                        Some(Token::Char(c)) => {
                            // If a character token that is one of U+0009 CHARACTER TABULATION, U+000A
                            // LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or
                            // U+0020 SPACE, ignore the token.
                            let num = c as u32;
                            if num == 0x09
                                || num == 0x0a
                                || num == 0x0c
                                || num == 0x0d
                                || num == 0x20
                            {
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(Token::StartTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            // A start tag whose tag name is "html"
                            // Create an element for the token in the HTML namespace, with the Document
                            // as the intended parent. Append it to the Document object. Put this
                            // element in the stack of open elements.
                            if tag == "html" {
                                self.append_to_root(&mut root, tag);
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(Token::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            // Any other end tag
                            // Parse error. Ignore the token.
                            if tag != "head" || tag != "body" || tag != "html" || tag != "br" {
                                // Ignore the token.
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(Token::Eof) | None => {
                            return root;
                        }
                    }
                    self.append_to_root(&mut root, "html");
                    self.mode = InsertionMode::BeforeHead;
                } // end of InsertionMode::BeforeHtml

                // https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
                InsertionMode::BeforeHead => {
                    match token {
                        Some(Token::Char(c)) => {
                            let num = c as u32;
                            // If a character token that is one of U+0009 CHARACTER TABULATION, U+000A
                            // LINE FEED (LF), U+000C FORM FEED (FF), U+000D CARRIAGE RETURN (CR), or
                            // U+0020 SPACE, ignore the token.
                            if num == 0x09
                                || num == 0x0a
                                || num == 0x0c
                                || num == 0x0d
                                || num == 0x20
                            {
                                token = self.t.next();
                            }
                        }
                        Some(Token::StartTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "head" {
                                // TODO: add head node to the tree.
                            }
                        }
                        _ => {}
                    }
                    // TODO: add head node to the tree.
                    self.mode = InsertionMode::InHead;
                } // end of InsertionMode::BeforeHead
                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
                InsertionMode::InHead => {} // end of InsertionMode::InHead
                // https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
                InsertionMode::AfterHead => {} // end of InsertionMode::AfterHead
                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
                InsertionMode::InBody => {} // end of InsertionMode::InBody
                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-afterbody
                InsertionMode::AfterBody => {} // end of InsertionMode::AfterBody
                // https://html.spec.whatwg.org/multipage/parsing.html#the-after-after-body-insertion-mode
                InsertionMode::AfterAfterBody => {} // end of InsertionMode::AfterAfterBody
            } // end of match self.mode {}
        } // end of while token.is_some {}

        root
    }
}
