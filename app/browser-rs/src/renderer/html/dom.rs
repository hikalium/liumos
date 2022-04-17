//! This is a part of "13.2.6 Tree construction" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

use crate::alloc::string::ToString;
use crate::renderer::html::token::{HtmlToken, HtmlTokenizer};
use crate::renderer::html::Attribute;
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

    #[allow(dead_code)]
    pub fn last_child(&self) -> Option<Weak<RefCell<Node>>> {
        self.last_child.as_ref().map(|n| n.clone())
    }

    #[allow(dead_code)]
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
    pub kind: ElementKind,
    pub attributes: Vec<Attribute>,
}

impl Element {
    pub fn new(kind: ElementKind, attributes: Vec<Attribute>) -> Self {
        Self { kind, attributes }
    }

    // associated function
    fn str_to_element_kind(name: &str) -> ElementKind {
        if name == "html" {
            ElementKind::Html
        } else if name == "head" {
            ElementKind::Head
        } else if name == "link" {
            ElementKind::Link
        } else if name == "style" {
            ElementKind::Style
        } else if name == "script" {
            ElementKind::Script
        } else if name == "body" {
            ElementKind::Body
        } else if name == "ul" {
            ElementKind::Ul
        } else if name == "li" {
            ElementKind::Li
        } else if name == "div" {
            ElementKind::Div
        } else {
            unimplemented!("not supported this tag name: {}", name);
        }
    }

    // associated function
    pub fn element_kind_to_string(kind: ElementKind) -> String {
        if kind == ElementKind::Html {
            "html".to_string()
        } else if kind == ElementKind::Head {
            "head".to_string()
        } else if kind == ElementKind::Link {
            "link".to_string()
        } else if kind == ElementKind::Style {
            "style".to_string()
        } else if kind == ElementKind::Script {
            "script".to_string()
        } else if kind == ElementKind::Body {
            "body".to_string()
        } else if kind == ElementKind::Ul {
            "ul".to_string()
        } else if kind == ElementKind::Li {
            "li".to_string()
        } else if kind == ElementKind::Div {
            "div".to_string()
        } else {
            unimplemented!("not supported this element kind: {:?}", kind);
        }
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
    /// https://html.spec.whatwg.org/multipage/scripting.html#the-script-element
    Script,
    /// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-incdata
    Text,
    /// https://html.spec.whatwg.org/multipage/sections.html#the-body-element
    Body,
    /// https://html.spec.whatwg.org/multipage/grouping-content.html#the-ul-element
    Ul,
    /// https://html.spec.whatwg.org/multipage/grouping-content.html#the-li-element
    Li,
    /// https://html.spec.whatwg.org/multipage/grouping-content.html#the-div-element
    Div,
}

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

    /// Creates a char node.
    fn create_char(&self, c: char) -> Node {
        let mut s = String::new();
        s.push(c);
        return Node::new(NodeKind::Text(s));
    }

    /// Creates an element node.
    fn create_element(&self, tag: &str, attributes: Vec<Attribute>) -> Node {
        let kind = Element::str_to_element_kind(tag);

        return Node::new(NodeKind::Element(Element::new(kind, attributes)));
    }

    /// Creates an element node for the token and insert it to the appropriate place for inserting
    /// a node. Put the new node in the stack of open elements.
    /// https://html.spec.whatwg.org/multipage/parsing.html#insert-a-foreign-element
    fn insert_element(&mut self, tag: &str, attributes: Vec<Attribute>) {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => &self.root,
        };

        let node = Rc::new(RefCell::new(self.create_element(tag, attributes)));

        if current.borrow().first_child().is_some() {
            let mut last_sibiling = current.borrow().first_child();
            loop {
                last_sibiling = match last_sibiling {
                    Some(ref node) => {
                        if node.borrow().next_sibling().is_some() {
                            node.borrow().next_sibling()
                        } else {
                            break;
                        }
                    }
                    None => unimplemented!("last_sibiling should be Some"),
                };
            }

            last_sibiling.unwrap().borrow_mut().next_sibling = Some(node.clone());
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

        // do not create a Text node if new char is '\n' or ' '
        if c == '\n' || c == ' ' {
            return;
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

    /// Returns true if the current node's kind is same as NodeKind::Element::<element_kind>.
    fn pop_current_node(&mut self, element_kind: ElementKind) -> bool {
        let current = match self.stack_of_open_elements.last() {
            Some(n) => n,
            None => return false,
        };

        if current.borrow().kind == NodeKind::Element(Element::new(element_kind, Vec::new())) {
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

            if current.borrow().kind == NodeKind::Element(Element::new(element_kind, Vec::new())) {
                return;
            }
        }
    }

    /// Returns true if the stack of open elements has NodeKind::Element::<element_kind> node.
    fn contain_in_stack(&mut self, element_kind: ElementKind) -> bool {
        for i in 0..self.stack_of_open_elements.len() {
            if self.stack_of_open_elements[i].borrow().kind
                == NodeKind::Element(Element::new(element_kind, Vec::new()))
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
                        Some(HtmlToken::Char(c)) => {
                            if c == ' ' || c == '\n' {
                                token = self.t.next();
                                continue;
                            }
                        }
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            ref attributes,
                        }) => {
                            // A start tag whose tag name is "html"
                            // Create an element for the token in the HTML namespace, with the Document
                            // as the intended parent. Append it to the Document object. Put this
                            // element in the stack of open elements.
                            if tag == "html" {
                                self.insert_element(tag, attributes.to_vec());
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
                    self.insert_element("html", Vec::new());
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
                            ref attributes,
                        }) => {
                            if tag == "head" {
                                self.insert_element(tag, attributes.to_vec());
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
                    self.insert_element("head", Vec::new());
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
                                self.insert_element("link", attributes.to_vec());
                                // Immediately pop the current node off the stack of open elements.
                                assert!(self.pop_current_node(ElementKind::Link));
                                token = self.t.next();
                                continue;
                            }

                            if tag == "style" {
                                self.insert_element(tag, attributes.to_vec());
                                self.original_insertion_mode = self.mode;
                                self.mode = InsertionMode::Text;
                                token = self.t.next();
                                continue;
                            }

                            if tag == "script" {
                                self.insert_element(tag, attributes.to_vec());
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
                            ref attributes,
                        }) => {
                            if tag == "body" {
                                self.insert_element(tag, attributes.to_vec());
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
                    self.insert_element("body", Vec::new());
                    self.mode = InsertionMode::InBody;
                } // end of InsertionMode::AfterHead

                // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
                InsertionMode::InBody => {
                    match token {
                        Some(HtmlToken::StartTag {
                            ref tag,
                            self_closing: _,
                            ref attributes,
                        }) => {
                            if tag == "ul" {
                                self.insert_element(tag, attributes.to_vec());
                                token = self.t.next();
                                continue;
                            }
                            if tag == "li" {
                                self.insert_element(tag, attributes.to_vec());
                                token = self.t.next();
                                continue;
                            }
                            if tag == "div" {
                                self.insert_element(tag, attributes.to_vec());
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
                            if tag == "div" {
                                token = self.t.next();
                                self.pop_until(ElementKind::Div);
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

                            if tag == "script" {
                                self.pop_until(ElementKind::Script);
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

fn get_target_element_node(
    node: Option<Rc<RefCell<Node>>>,
    element_kind: ElementKind,
) -> Option<Rc<RefCell<Node>>> {
    match node {
        Some(n) => {
            if n.borrow().kind == NodeKind::Element(Element::new(element_kind, Vec::new())) {
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

pub fn get_style_content(root: Rc<RefCell<Node>>) -> String {
    let style_node = match get_target_element_node(Some(root), ElementKind::Style) {
        Some(node) => node,
        None => return "".to_string(),
    };
    let text_node = match style_node.borrow().first_child() {
        Some(node) => node,
        None => return "".to_string(),
    };
    let content = match &text_node.borrow().kind {
        NodeKind::Text(ref s) => s.clone(),
        _ => "".to_string(),
    };
    content
}

pub fn get_js_content(root: Rc<RefCell<Node>>) -> String {
    let js_node = match get_target_element_node(Some(root), ElementKind::Script) {
        Some(node) => node,
        None => return "".to_string(),
    };
    let text_node = match js_node.borrow().first_child() {
        Some(node) => node,
        None => return "".to_string(),
    };
    let content = match &text_node.borrow().kind {
        NodeKind::Text(ref s) => s.clone(),
        _ => "".to_string(),
    };
    content
}
