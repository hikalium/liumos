use super::cssom::*;
use super::dom::*;
use crate::gui::ApplicationWindow;
use alloc::rc::{Rc, Weak};
use alloc::vec::Vec;
use core::cell::RefCell;
#[allow(unused_imports)]
use liumlib::*;

#[derive(Debug, Clone)]
#[allow(dead_code)]
struct RenderObject {
    // Similar structure with Node in renderer/dom.rs.
    kind: NodeKind,
    first_child: Option<Rc<RefCell<Node>>>,
    last_child: Option<Weak<RefCell<Node>>>,
    previous_sibling: Option<Weak<RefCell<Node>>>,
    next_sibling: Option<Rc<RefCell<Node>>>,
    // CSS info.
    style: Vec<Declaration>,
}

#[allow(dead_code)]
impl RenderObject {
    fn new(root: Rc<RefCell<Node>>) -> Self {
        let borrowed_root = root.borrow();
        Self {
            kind: borrowed_root.kind.clone(),
            first_child: borrowed_root.first_child().clone(),
            last_child: borrowed_root.last_child().clone(),
            previous_sibling: borrowed_root.previous_sibling().clone(),
            next_sibling: borrowed_root.next_sibling().clone(),
            style: Vec::new(),
        }
    }

    fn add_style(&mut self, declaration: Declaration) {
        self.style.push(declaration);
    }
}

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct RenderTree {
    root: RenderObject,
}

#[allow(dead_code)]
impl RenderTree {
    pub fn new(root: Rc<RefCell<Node>>) -> Self {
        Self {
            root: RenderObject::new(root),
        }
    }

    fn dfs(&self, node: Option<Rc<RefCell<Node>>>) {
        match node {
            Some(n) => {
                self.dfs(n.borrow().first_child());
                self.dfs(n.borrow().next_sibling());
            }
            None => return,
        }
    }

    pub fn apply(&mut self, _cssom: &StyleSheet) {}

    pub fn paint(&self, _window: &ApplicationWindow) {}
}
