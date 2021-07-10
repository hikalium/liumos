//! This is a part of "13.2.6 Tree construction" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

use crate::parser::tokenizer::*;
#[allow(unused_imports)]
use alloc::string::String;
#[allow(unused_imports)]
use alloc::vec::Vec;
#[allow(unused_imports)]
use core::assert;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum Node {
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

#[allow(dead_code)]
///dom.spec.whatwg.org/#interface-node
impl Node {
    fn first_child(&self) -> Option<Node> {
        None
    }
    fn last_child(&self) -> Option<Node> {
        None
    }
    fn previous_sibling(&self) -> Option<Node> {
        None
    }
    fn next_sibling(&self) -> Option<Node> {
        None
    }
}

/// https://html.spec.whatwg.org/multipage/semantics.html#htmlhtmlelement
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct HtmlElementImpl {}

impl HtmlElementImpl {
    #[allow(dead_code)]
    pub fn new() -> Self {
        Self {}
    }
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
enum NodeType {
    Element = 1,
    Attr = 2,
    Text = 3,
    Comment = 8,
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
    Text,
    AfterBody,
    AfterAfterBody,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Parser {
    mode: InsertionMode,
    t: Tokenizer,
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct DomTree {
    root: Node,
}

impl DomTree {
    #[allow(dead_code)]
    fn new() -> Self {
        Self {
            root: Node::Document,
        }
    }

    #[allow(dead_code)]
    pub fn root(&self) -> Node {
        self.root
    }
}

impl Parser {
    #[allow(dead_code)]
    pub fn new(t: Tokenizer) -> Self {
        Self {
            mode: InsertionMode::Initial,
            t,
        }
    }

    #[allow(dead_code)]
    pub fn construct_tree(&mut self) -> DomTree {
        let tree = DomTree::new();

        let _token = match self.t.next() {
            Some(t) => t,
            None => return tree,
        };

        match self.mode {
            // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
            InsertionMode::Initial => self.mode = InsertionMode::BeforeHtml,
            // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
            InsertionMode::BeforeHtml => {}
            _ => {}
        }

        tree
    }
}
