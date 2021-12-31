use crate::gui::ApplicationWindow;
use crate::renderer::css::box_model::BoxInfo;
use crate::renderer::css::cssom::*;
use crate::renderer::html::dom::*;
use crate::renderer::NodeKind::Element;
use alloc::rc::{Rc, Weak};
use alloc::vec::Vec;
use core::cell::RefCell;
use liumlib::gui::draw_rect;
use liumlib::gui::BitmapImageBuffer;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone)]
pub struct RenderObject {
    // Similar structure with Node in renderer/dom.rs.
    pub kind: NodeKind,
    first_child: Option<Rc<RefCell<RenderObject>>>,
    last_child: Option<Weak<RefCell<RenderObject>>>,
    previous_sibling: Option<Weak<RefCell<RenderObject>>>,
    next_sibling: Option<Rc<RefCell<RenderObject>>>,
    // CSS information.
    pub style: Vec<Declaration>,
    box_info: BoxInfo,
}

impl RenderObject {
    fn new(node: Rc<RefCell<Node>>) -> Self {
        Self {
            kind: node.borrow().kind.clone(),
            first_child: None,
            last_child: None,
            previous_sibling: None,
            next_sibling: None,
            style: Vec::new(),
            box_info: BoxInfo::new(),
        }
    }

    pub fn first_child(&self) -> Option<Rc<RefCell<RenderObject>>> {
        self.first_child.as_ref().map(|n| n.clone())
    }

    pub fn next_sibling(&self) -> Option<Rc<RefCell<RenderObject>>> {
        self.next_sibling.as_ref().map(|n| n.clone())
    }
}

#[derive(Debug, Clone)]
pub struct RenderTree {
    pub root: Option<Rc<RefCell<RenderObject>>>,
}

impl RenderTree {
    pub fn new(root: Rc<RefCell<Node>>) -> Self {
        Self {
            root: Self::dom_to_render_tree(&Some(root)),
        }
    }

    fn check_kind(node_kind: &NodeKind, selector: &Selector) -> bool {
        match node_kind {
            Element(e) => match selector {
                Selector::TypeSelector(s) => {
                    if e.kind == ElementKind::Div && s == "div" {
                        return true;
                    }
                    return false;
                }
                _ => return false,
            },
            _ => false,
        }
    }

    fn dom_to_render_object(node: &Option<Rc<RefCell<Node>>>) -> Option<Rc<RefCell<RenderObject>>> {
        match node {
            Some(n) => Some(Rc::new(RefCell::new(RenderObject::new(n.clone())))),
            None => None,
        }
    }

    fn dom_to_render_tree(root: &Option<Rc<RefCell<Node>>>) -> Option<Rc<RefCell<RenderObject>>> {
        let render_object = Self::dom_to_render_object(&root);

        let obj = match render_object {
            Some(ref obj) => obj,
            None => return None,
        };

        match root {
            Some(n) => {
                let first_child = Self::dom_to_render_tree(&n.borrow().first_child());
                let next_sibling = Self::dom_to_render_tree(&n.borrow().next_sibling());

                obj.borrow_mut().first_child = first_child;
                obj.borrow_mut().next_sibling = next_sibling;
            }
            None => return None,
        }

        return render_object;
    }

    fn apply_rule_to_render_object(
        node: &Option<Rc<RefCell<RenderObject>>>,
        css_rule: &QualifiedRule,
    ) {
        match node {
            Some(n) => {
                if Self::check_kind(&n.borrow().kind, &css_rule.selector) {
                    n.borrow_mut().style = css_rule.declarations.clone();
                }

                Self::apply_rule_to_render_object(&n.borrow().first_child(), css_rule);
                Self::apply_rule_to_render_object(&n.borrow().next_sibling(), css_rule);
            }
            None => return,
        }
    }

    /// Apply CSS Object Model to RenderTree.
    pub fn apply(&mut self, cssom: &StyleSheet) {
        for rule in &cssom.rules {
            Self::apply_rule_to_render_object(&self.root, rule);
        }
    }

    fn paint_node(&self, window: &ApplicationWindow, node: &Option<Rc<RefCell<RenderObject>>>) {
        match node {
            Some(n) => {
                let mut background_color = 0xffffff; // default color

                for style in &n.borrow().style {
                    match style.property.as_str() {
                        "background-color" => {
                            background_color = match style.value.as_str() {
                                "red" => 0xff0000,
                                "green" => 0x00ff00,
                                "blue" => 0x0000ff,
                                _ => 0xffffff,
                            };
                        }
                        _ => {}
                    }
                }

                match &n.borrow().kind {
                    NodeKind::Document => {}
                    NodeKind::Element(element) => {
                        match element.kind {
                            ElementKind::Html
                            | ElementKind::Head
                            | ElementKind::Style
                            | ElementKind::Script
                            | ElementKind::Body => {}
                            // TODO: support <a>
                            ElementKind::Link => {}
                            // TODO: support raw text
                            ElementKind::Text => {}
                            // TODO: support <ul>
                            ElementKind::Ul => {}
                            // TODO: support <li>
                            ElementKind::Li => {}
                            // TODO: support <div>
                            ElementKind::Div => {
                                draw_rect(
                                    &window.buffer,
                                    background_color,
                                    window.content_x,
                                    window.content_y,
                                    window.content_w,
                                    window.content_h,
                                )
                                .expect("draw a div");
                                window.buffer.flush();
                            }
                        }
                    }
                    NodeKind::Text(_text) => {}
                }

                self.paint_node(window, &n.borrow().first_child());
                self.paint_node(window, &n.borrow().next_sibling());
            }
            None => return,
        }
    }

    /// Paint the current RenderTree to ApplicationWindow.
    pub fn paint(&self, window: &ApplicationWindow) {
        self.paint_node(window, &self.root);
    }
}
