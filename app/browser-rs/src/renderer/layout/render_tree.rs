//! https://www.w3.org/TR/css-box-3/
//! https://www.w3.org/TR/css-layout-api-1/

use crate::alloc::string::ToString;
use crate::gui::ApplicationWindow;
use crate::renderer::css::cssom::*;
use crate::renderer::html::dom::*;
use crate::renderer::{Element, NodeKind};
use alloc::rc::{Rc, Weak};
use alloc::string::String;
use alloc::vec::Vec;
use core::cell::RefCell;
use liumlib::gui::draw_rect;
use liumlib::gui::BitmapImageBuffer;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct RenderStyle {
    background_color: u32,
    color: u32,
    display: DisplayType,
    text_align: String,
    // TODO: support string (e.g. "auto")
    height: u64,
    width: u64,
    margin: BoxInfo,
    padding: BoxInfo,
    // witdh, style, color
    border: (String, String, String),
}

impl RenderStyle {
    pub fn new(node: &Rc<RefCell<Node>>) -> Self {
        Self {
            background_color: 0xffffff, // white
            color: 0xffffff,            // white
            display: Self::display_type(node),
            text_align: "left".to_string(),
            width: 0,
            height: 0,
            margin: BoxInfo::new(0, 0, 0, 0),
            padding: BoxInfo::new(0, 0, 0, 0),
            border: (
                "medium".to_string(),
                "none".to_string(),
                "color".to_string(),
            ),
        }
    }

    fn display_type(node: &Rc<RefCell<Node>>) -> DisplayType {
        match &node.borrow().kind {
            NodeKind::Element(element) => match element.kind {
                ElementKind::Div | ElementKind::Ul | ElementKind::Li => DisplayType::Block,
                _ => DisplayType::Inline,
            },
            _ => DisplayType::Inline,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum DisplayType {
    Block,
    Inline,
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct BoxInfo {
    top: u64,
    right: u64,
    left: u64,
    bottom: u64,
}

impl BoxInfo {
    fn new(top: u64, right: u64, left: u64, bottom: u64) -> Self {
        Self {
            top,
            right,
            left,
            bottom,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct LayoutPosition {
    x: u64,
    y: u64,
}

impl LayoutPosition {
    fn new(x: u64, y: u64) -> Self {
        Self { x, y }
    }
}

#[derive(Debug, Clone)]
pub struct RenderObject {
    // Similar structure with Node in renderer/dom.rs.
    pub kind: NodeKind,
    first_child: Option<Rc<RefCell<RenderObject>>>,
    last_child: Option<Weak<RefCell<RenderObject>>>,
    previous_sibling: Option<Weak<RefCell<RenderObject>>>,
    next_sibling: Option<Rc<RefCell<RenderObject>>>,
    // CSS information.
    pub style: RenderStyle,
    // Layout information.
    position: LayoutPosition,
}

impl RenderObject {
    fn new(node: Rc<RefCell<Node>>) -> Self {
        Self {
            kind: node.borrow().kind.clone(),
            first_child: None,
            last_child: None,
            previous_sibling: None,
            next_sibling: None,
            style: RenderStyle::new(&node),
            position: LayoutPosition::new(0, 0),
        }
    }

    pub fn first_child(&self) -> Option<Rc<RefCell<RenderObject>>> {
        self.first_child.as_ref().map(|n| n.clone())
    }

    pub fn next_sibling(&self) -> Option<Rc<RefCell<RenderObject>>> {
        self.next_sibling.as_ref().map(|n| n.clone())
    }

    pub fn set_style(&mut self, declarations: Vec<Declaration>) {
        for declaration in declarations {
            match declaration.property.as_str() {
                "background-color" => {
                    self.style.background_color = match declaration.value {
                        ComponentValue::Keyword(value) => match value.as_str() {
                            "red" => 0xff0000,
                            "orange" => 0xffa500,
                            "yellow" => 0xffff00,
                            "green" => 0x008000,
                            "blue" => 0x0000ff,
                            "purple" => 0x800080,
                            "pink" => 0xffc0cb,
                            "lightgray" => 0xd3d3d3,
                            "gray" => 0x808080,
                            "black" => 0x000000,
                            _ => 0xffffff,
                        },
                        _ => 0xffffff,
                    };
                }
                "height" => {
                    self.style.height = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => 0,
                        ComponentValue::Number(value) => value,
                    };
                }
                "width" => {
                    self.style.width = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => 0,
                        ComponentValue::Number(value) => value,
                    };
                }
                "margin" => {
                    self.style.margin = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => self.style.margin.clone(),
                        ComponentValue::Number(value) => BoxInfo::new(value, value, value, value),
                    };
                }
                "margin-top" => {
                    self.style.margin.top = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => self.style.margin.top,
                        ComponentValue::Number(value) => value,
                    };
                }
                "margin-right" => {
                    self.style.margin.right = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => self.style.margin.right,
                        ComponentValue::Number(value) => value,
                    };
                }
                "margin-bottom" => {
                    self.style.margin.bottom = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => self.style.margin.bottom,
                        ComponentValue::Number(value) => value,
                    };
                }
                "margin-left" => {
                    self.style.margin.left = match declaration.value {
                        // TODO: support string (e.g. "auto")
                        ComponentValue::Keyword(_value) => self.style.margin.left,
                        ComponentValue::Number(value) => value,
                    };
                }
                _ => unimplemented!("css property {} is not supported yet", declaration.property,),
            }
        }
    }

    fn layout(&mut self, parent_style: &RenderStyle, parent_position: &LayoutPosition) {
        match parent_style.display {
            DisplayType::Inline => {
                match self.style.display {
                    DisplayType::Block => {
                        // TODO: set position property
                        self.position.x = self.style.margin.left;
                        self.position.y = self.style.margin.top + parent_style.height;
                    }
                    DisplayType::Inline => {
                        self.position.x = parent_position.x + parent_style.width;
                        self.position.y = parent_position.y;
                    }
                }
            }
            DisplayType::Block => {
                match self.style.display {
                    DisplayType::Block => {
                        self.position.x = self.style.margin.left;
                        self.position.y = parent_position.y
                            + parent_style.height
                            + parent_style.margin.bottom
                            + self.style.margin.top;
                    }
                    DisplayType::Inline => {
                        // TODO: set position property
                        self.position.x = 0;
                        self.position.y = parent_style.height;
                    }
                }
            }
        }
    }

    fn paint(&self, window: &ApplicationWindow) {
        match &self.kind {
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
                            self.style.background_color,
                            window.content_x + self.position.x as i64,
                            window.content_y + self.position.y as i64,
                            self.style.width as i64,
                            self.style.height as i64,
                        )
                        .expect("draw a div");
                        window.buffer.flush();
                    }
                }
            }
            NodeKind::Text(_text) => {}
        }
    }
}

#[derive(Debug, Clone)]
pub struct RenderTree {
    pub root: Option<Rc<RefCell<RenderObject>>>,
}

impl RenderTree {
    pub fn new(root: Rc<RefCell<Node>>, cssom: &StyleSheet) -> Self {
        let mut tree = Self {
            root: Self::dom_to_render_tree(&Some(root)),
        };

        tree.apply(cssom);
        tree.layout();

        tree
    }

    fn is_node_selected(&self, node_kind: &NodeKind, selector: &Selector) -> bool {
        match node_kind {
            NodeKind::Element(e) => match selector {
                Selector::TypeSelector(type_name) => {
                    if Element::element_kind_to_string(e.kind) == *type_name {
                        return true;
                    }
                    return false;
                }
                Selector::ClassSelector(class_name) => {
                    for attr in &e.attributes {
                        if attr.name == "class" && attr.value == *class_name {
                            return true;
                        }
                    }
                    return false;
                }
                Selector::IdSelector(id_name) => {
                    for attr in &e.attributes {
                        if attr.name == "id" && attr.value == *id_name {
                            return true;
                        }
                    }
                    return false;
                }
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

    /// Converts DOM tree to render tree.
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
        &self,
        node: &Option<Rc<RefCell<RenderObject>>>,
        css_rule: &QualifiedRule,
    ) {
        match node {
            Some(n) => {
                if self.is_node_selected(&n.borrow().kind, &css_rule.selector) {
                    n.borrow_mut().set_style(css_rule.declarations.clone());
                }

                self.apply_rule_to_render_object(&n.borrow().first_child(), css_rule);
                self.apply_rule_to_render_object(&n.borrow().next_sibling(), css_rule);
            }
            None => return,
        }
    }

    /// Applys CSS Object Model to RenderTree.
    fn apply(&mut self, cssom: &StyleSheet) {
        for rule in &cssom.rules {
            self.apply_rule_to_render_object(&self.root, rule);
        }
    }

    fn layout_node(
        &self,
        node: &Option<Rc<RefCell<RenderObject>>>,
        parent_style: &RenderStyle,
        parent_position: &LayoutPosition,
    ) {
        match node {
            Some(n) => {
                n.borrow_mut().layout(parent_style, parent_position);

                let first_child = n.borrow().first_child();
                self.layout_node(&first_child, &n.borrow().style, &n.borrow().position);

                let next_sibling = n.borrow().next_sibling();
                self.layout_node(&next_sibling, &n.borrow().style, &n.borrow().position);
            }
            None => return,
        }
    }

    /// Calculate the layout position.
    fn layout(&mut self) {
        let fake_node = Rc::new(RefCell::new(Node::new(NodeKind::Document)));
        let fake_style = RenderStyle::new(&fake_node);
        let fake_position = LayoutPosition::new(0, 0);
        self.layout_node(&self.root, &fake_style, &fake_position);
    }

    fn paint_node(&self, window: &ApplicationWindow, node: &Option<Rc<RefCell<RenderObject>>>) {
        match node {
            Some(n) => {
                n.borrow().paint(window);

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
