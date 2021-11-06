//! This is a part of "13.2.6 Tree construction" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

use crate::renderer::html_token::*;
use alloc::rc::{Rc, Weak};
use alloc::string::String;
use alloc::vec::Vec;
use core::cell::RefCell;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone)]
/// https://dom.spec.whatwg.org/#interface-node
pub struct Node {
    pub kind: NodeKind,
    pub parent: Option<Weak<RefCell<Node>>>,
    pub first_child: Option<Rc<RefCell<Node>>>,
    pub last_child: Option<Weak<RefCell<Node>>>,
    pub previous_sibling: Option<Weak<RefCell<Node>>>,
    pub next_sibling: Option<Rc<RefCell<Node>>>,
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

    pub fn first_child(&self) -> Option<Rc<RefCell<Node>>> {
        self.first_child.as_ref().map(|n| n.clone())
    }

    pub fn last_child(&self) -> Option<Weak<RefCell<Node>>> {
        self.last_child.as_ref().map(|n| n.clone())
    }

    pub fn previous_sibling(&self) -> Option<Weak<RefCell<Node>>> {
        self.previous_sibling.as_ref().map(|n| n.clone())
    }

    pub fn next_sibling(&self) -> Option<Rc<RefCell<Node>>> {
        self.next_sibling.as_ref().map(|n| n.clone())
    }
}

#[derive(Debug, Clone)]
pub enum NodeKind {
    /// https://dom.spec.whatwg.org/#interface-document
    Document,
    /// https://dom.spec.whatwg.org/#interface-element
    Element(Element),
    /// https://dom.spec.whatwg.org/#interface-text
    Text(String),
}

impl PartialEq for NodeKind {
    fn eq(&self, other: &Self) -> bool {
        match &self {
            NodeKind::Document => match &other {
                NodeKind::Document => true,
                _ => false,
            },
            NodeKind::Element(e1) => match &other {
                NodeKind::Element(e2) => e1.kind == e2.kind,
                _ => false,
            },
            NodeKind::Text(_) => match &other {
                NodeKind::Text(_) => true,
                _ => false,
            },
        }
    }
}

impl Eq for NodeKind {}

#[derive(Debug, Clone, PartialEq, Eq)]
/// https://dom.spec.whatwg.org/#interface-element
pub struct Element {
    kind: ElementKind,
    attributes: Vec<Attribute>,
}

impl Element {
    pub fn new(kind: ElementKind) -> Self {
        Self {
            kind,
            attributes: Vec::new(),
        }
    }

    pub fn set_attribute(&mut self, name: String, value: String) {
        self.attributes.push(Attribute::new(name, value));
    }
}

#[allow(dead_code)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
/// https://dom.spec.whatwg.org/#interface-element
pub enum ElementKind {
    /// https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
    Html,
    /// https://html.spec.whatwg.org/multipage/semantics.html#the-head-element
    Head,
    /// https://html.spec.whatwg.org/multipage/semantics.html#the-link-element
    Link,
    /// https://html.spec.whatwg.org/multipage/semantics.html#the-style-element
    Style,
    /// https://html.spec.whatwg.org/multipage/sections.html#the-body-element
    Body,
    /// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
    Text,
    /// https://html.spec.whatwg.org/multipage/grouping-content.html#the-ul-element
    Ul,
    /// https://html.spec.whatwg.org/multipage/grouping-content.html#the-li-element
    Li,
}

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
/// https://dom.spec.whatwg.org/#attr
pub struct Attribute {
    name: String,
    value: String,
}

impl Attribute {
    pub fn new(name: String, value: String) -> Self {
        Self { name, value }
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
    Text,
    AfterBody,
    AfterAfterBody,
}

#[derive(Debug, Clone)]
pub struct HtmlParser {
    root: Rc<RefCell<Node>>,
    mode: InsertionMode,
    t: HtmlTokenizer,
    /// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
    stack_of_open_elements: Vec<Rc<RefCell<Node>>>,
    /// https://html.spec.whatwg.org/multipage/parsing.html#original-insertion-mode
    original_insertion_mode: InsertionMode,
}

impl HtmlParser {
    pub fn new(t: HtmlTokenizer) -> Self {
        Self {
            root: Rc::new(RefCell::new(Node::new(NodeKind::Document))),
            mode: InsertionMode::Initial,
            t,
            stack_of_open_elements: Vec::new(),
            original_insertion_mode: InsertionMode::Initial,
        }
    }

    /// Creates an element node.
    fn create_element(&self, kind: ElementKind) -> Node {
        return Node::new(NodeKind::Element(Element::new(kind)));
    }

    /// Creates a char node.
    fn create_char(&self, c: char) -> Node {
        let mut s = String::new();
        s.push(c);
        return Node::new(NodeKind::Text(s));
    }

    /// Creates an element based on the `tag` string.
    fn create_element_by_tag(&self, tag: &str) -> Node {
        if tag == "html" {
            return self.create_element(ElementKind::Html);
        } else if tag == "head" {
            return self.create_element(ElementKind::Head);
        } else if tag == "link" {
            return self.create_element(ElementKind::Link);
        } else if tag == "style" {
            return self.create_element(ElementKind::Style);
        } else if tag == "body" {
            return self.create_element(ElementKind::Body);
        } else if tag == "ul" {
            return self.create_element(ElementKind::Ul);
        } else if tag == "li" {
            return self.create_element(ElementKind::Li);
        } else if tag == "div" {
            return self.create_element(ElementKind::Li);
        }
        panic!("not supported this tag name: {}", tag);
    }

    /// Creates an element node for the token and insert it to the appropriate place for inserting
    /// a node. Put the new node in the stack of open elements.
    /// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
    fn insert_element(&mut self, tag: &str) {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => &self.root,
        };

        let node = Rc::new(RefCell::new(self.create_element_by_tag(tag)));

        if current.borrow().first_child().is_some() {
            current
                .borrow()
                .first_child()
                .unwrap()
                .borrow_mut()
                .next_sibling = Some(node.clone());
            node.borrow_mut().previous_sibling =
                Some(Rc::downgrade(&current.borrow().first_child().unwrap()));
        } else {
            current.borrow_mut().first_child = Some(node.clone());
        }

        current.borrow_mut().last_child = Some(Rc::downgrade(&node));
        node.borrow_mut().parent = Some(Rc::downgrade(&current));

        self.stack_of_open_elements.push(node);
    }

    /// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-character
    fn insert_char(&mut self, c: char) {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => &self.root,
        };

        // When the current node is Text, add a character to the current node.
        match current.borrow_mut().kind {
            NodeKind::Text(ref mut s) => {
                s.push(c);
                return;
            }
            _ => {}
        }

        let node = Rc::new(RefCell::new(self.create_char(c)));

        if current.borrow().first_child().is_some() {
            current
                .borrow()
                .first_child()
                .unwrap()
                .borrow_mut()
                .next_sibling = Some(node.clone());
            node.borrow_mut().previous_sibling =
                Some(Rc::downgrade(&current.borrow().first_child().unwrap()));
        } else {
            current.borrow_mut().first_child = Some(node.clone());
        }

        current.borrow_mut().last_child = Some(Rc::downgrade(&node));
        node.borrow_mut().parent = Some(Rc::downgrade(&current));

        self.stack_of_open_elements.push(node);
    }

    /// Sets an attribute to the current node.
    fn set_attribute_to_current_node(&mut self, name: String, value: String) {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => &self.root,
        };

        match current.borrow_mut().kind {
            NodeKind::Element(ref mut elem) => {
                elem.set_attribute(name, value);
                return;
            }
            _ => {}
        }
    }

    /// Returns true if the current node's kind is same as NodeKind::Element::<element_kind>.
    fn pop_current_node(&mut self, element_kind: ElementKind) -> bool {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => return false,
        };

        if current.borrow().kind == NodeKind::Element(Element::new(element_kind)) {
            self.stack_of_open_elements.pop();
            return true;
        }

        false
    }

    /// Pops nodes until a node with `element_kind` comes.
    fn pop_until(&mut self, element_kind: ElementKind) {
        assert!(self.contain_in_stack(element_kind));

        loop {
            let current = match self.stack_of_open_elements.pop() {
                Some(n) => n,
                None => return,
            };

            if current.borrow().kind == NodeKind::Element(Element::new(element_kind)) {
                return;
            }
        }
    }

    /// Returns true if the stack of open elements has NodeKind::Element::<element_kind> node.
    fn contain_in_stack(&mut self, element_kind: ElementKind) -> bool {
        for i in 0..self.stack_of_open_elements.len() {
            if self.stack_of_open_elements[i].borrow().kind
                == NodeKind::Element(Element::new(element_kind))
            {
                return true;
            }
        }

        false
    }

    pub fn construct_tree(&mut self) -> Rc<RefCell<Node>> {
        let mut token = self.t.next();

        while token.is_some() {
            match self.mode {
                // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
                InsertionMode::Initial => self.mode = InsertionMode::BeforeHtml,

                // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
                InsertionMode::BeforeHtml => {
                    match token {
                        Some(HtmlToken::Doctype) => {
                            token = self.t.next();
                            continue;
                        }
                        Some(HtmlToken::Char(c)) => {
                            if c == ' ' || c == '\n' {
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            attributes: _,
                        }) => {
                            // A start tag whose tag name is "html"
                            // Create an element for the token in the HTML namespace, with the Document
                            // as the intended parent. Append it to the Document object. Put this
                            // element in the stack of open elements.
                            if tag == "html" {
                                self.insert_element(tag);
                                self.mode = InsertionMode::BeforeHead;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::EndTag {
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
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                    }
                    self.insert_element("html");
                    self.mode = InsertionMode::BeforeHead;
                } // end of InsertionMode::BeforeHtml

                // https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
                InsertionMode::BeforeHead => {
                    match token {
                        Some(HtmlToken::Char(c)) => {
                            if c == ' ' || c == '\n' {
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            attributes: _,
                        }) => {
                            if tag == "head" {
                                self.insert_element(tag);
                                self.mode = InsertionMode::InHead;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }
                    self.insert_element("head");
                    self.mode = InsertionMode::InHead;
                } // end of InsertionMode::BeforeHead

                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
                InsertionMode::InHead => {
                    match token {
                        Some(HtmlToken::Char(c)) => {
                            if c == ' ' || c == '\n' {
                                self.insert_char(c);
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            ref attributes,
                        }) => {
                            if tag == "link" {
                                self.insert_element("link");
                                for attr in attributes {
                                    self.set_attribute_to_current_node(
                                        attr.name.clone(),
                                        attr.value.clone(),
                                    );
                                }
                                // Immediately pop the current node off the stack of open elements.
                                assert!(self.pop_current_node(ElementKind::Link));
                                token = self.t.next();
                                continue;
                            }

                            if tag == "style" {
                                self.insert_element(tag);
                                self.original_insertion_mode = self.mode;
                                self.mode = InsertionMode::Text;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "head" {
                                self.mode = InsertionMode::AfterHead;
                                token = self.t.next();
                                self.pop_until(ElementKind::Head);
                                continue;
                            }
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }
                    self.mode = InsertionMode::AfterHead;
                    self.pop_until(ElementKind::Head);
                } // end of InsertionMode::InHead

                // https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
                InsertionMode::AfterHead => {
                    match token {
                        Some(HtmlToken::Char(c)) => {
                            if c == ' ' || c == '\n' {
                                self.insert_char(c);
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            attributes: _,
                        }) => {
                            if tag == "body" {
                                self.insert_element(tag);
                                token = self.t.next();
                                self.mode = InsertionMode::InBody;
                                continue;
                            }
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }
                    self.insert_element("body");
                    self.mode = InsertionMode::InBody;
                } // end of InsertionMode::AfterHead

                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
                InsertionMode::InBody => {
                    match token {
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            attributes: _,
                        }) => {
                            if tag == "ul" {
                                self.insert_element(tag);
                                token = self.t.next();
                                continue;
                            }
                            if tag == "li" {
                                self.insert_element(tag);
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "body" {
                                self.mode = InsertionMode::AfterBody;
                                token = self.t.next();
                                if !self.contain_in_stack(ElementKind::Body) {
                                    // Parse error. Ignore the token.
                                    continue;
                                }
                                self.pop_until(ElementKind::Body);
                                continue;
                            }
                            if tag == "html" {
                                // If the stack of open elements does not have a body element in
                                // scope, this is a parse error; ignore the token.
                                if self.pop_current_node(ElementKind::Body) {
                                    self.mode = InsertionMode::AfterBody;
                                    assert!(self.pop_current_node(ElementKind::Html));
                                } else {
                                    token = self.t.next();
                                }
                                continue;
                            }
                            if tag == "ul" {
                                token = self.t.next();
                                self.pop_until(ElementKind::Ul);
                                continue;
                            }
                            if tag == "li" {
                                token = self.t.next();
                                self.pop_until(ElementKind::Li);
                                continue;
                            }
                        }
                        Some(HtmlToken::Char(c)) => {
                            self.insert_char(c);
                            token = self.t.next();
                            continue;
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }
                } // end of InsertionMode::InBody

                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
                InsertionMode::Text => {
                    match token {
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        Some(HtmlToken::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "style" {
                                self.pop_until(ElementKind::Style);
                                self.mode = self.original_insertion_mode;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::Char(c)) => {
                            self.insert_char(c);
                            token = self.t.next();
                            continue;
                        }
                        _ => {}
                    }

                    self.mode = self.original_insertion_mode;
                } // end of InsertionMode::Text

                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-afterbody
                InsertionMode::AfterBody => {
                    match token {
                        Some(HtmlToken::Char(_c)) => {
                            // Not align with the spec.
                            // TODO: Process the token using the rules for the "in body" insertion
                            // mode.
                            token = self.t.next();
                            continue;
                        }
                        Some(HtmlToken::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "html" {
                                self.mode = InsertionMode::AfterAfterBody;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }

                    self.mode = InsertionMode::InBody;
                } // end of InsertionMode::AfterBody

                // https://html.spec.whatwg.org/multipage/parsing.html#the-after-after-body-insertion-mode
                InsertionMode::AfterAfterBody => {
                    match token {
                        Some(HtmlToken::Char(_c)) => {
                            // Not align with the spec.
                            // TODO: Process the token using the rules for the "in body" insertion
                            // mode.
                            token = self.t.next();
                            continue;
                        }
                        Some(HtmlToken::EndTag {
                            ref tag,
                            self_closing: _,
                        }) => {
                            if tag == "html" {
                                self.mode = InsertionMode::AfterAfterBody;
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::Eof) | None => {
                            return self.root.clone();
                        }
                        _ => {}
                    }

                    self.mode = InsertionMode::InBody;
                } // end of InsertionMode::AfterAfterBody
            } // end of match self.mode {}
        } // end of while token.is_some {}

        self.root.clone()
    }
}

#[allow(dead_code)]
fn get_target_element_node(
    node: Option<Rc<RefCell<Node>>>,
    element_kind: ElementKind,
) -> Option<Rc<RefCell<Node>>> {
    match node {
        Some(n) => {
            if n.borrow().kind == NodeKind::Element(Element::new(element_kind)) {
                return Some(n.clone());
            }
            let result1 = get_target_element_node(n.borrow().first_child(), element_kind);
            let result2 = get_target_element_node(n.borrow().next_sibling(), element_kind);
            if result1.is_none() && result2.is_none() {
                return None;
            }
            if result1.is_none() {
                return result2;
            }
            return result1;
        }
        None => return None,
    }
}

#[allow(dead_code)]
pub fn get_style_content(root: Rc<RefCell<Node>>) -> Option<String> {
    let style_node = get_target_element_node(Some(root), ElementKind::Style)?;
    let text_node = style_node.borrow().first_child()?;
    let content = match &text_node.borrow().kind {
        NodeKind::Text(ref s) => Some(s.clone()),
        _ => None,
    };
    content
}
