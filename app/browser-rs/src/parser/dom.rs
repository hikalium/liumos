//! This is a part of "13.2.6 Tree construction" in the HTML spec.
//! https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

#[allow(unused_imports)]
use alloc::string::String;
#[allow(unused_imports)]
use alloc::vec::Vec;
#[allow(unused_imports)]
use core::assert;
#[allow(unused_imports)]
use liumlib::*;

#[allow(dead_code)]
///dom.spec.whatwg.org/#interface-node
pub trait Node {
    fn first_child(&self) -> Option<&dyn Node>;
    fn last_child(&self) -> Option<&dyn Node>;
    fn previous_sibling(&self) -> Option<&dyn Node>;
    fn next_sibling(&self) -> Option<&dyn Node>;
}

#[allow(dead_code)]
pub struct NodeImpl {
    node_type: NodeType,
}

#[allow(dead_code)]
enum NodeType {
    Element = 1,
    Attr = 2,
    Text = 3,
    Comment = 8,
}

#[allow(dead_code)]
impl NodeImpl {
    fn new(node_type: NodeType) -> Self {
        Self { node_type }
    }
}

#[allow(dead_code)]
impl Node for NodeImpl {
    fn first_child(&self) -> Option<&dyn Node> {
        None
    }
    fn last_child(&self) -> Option<&dyn Node> {
        None
    }
    fn previous_sibling(&self) -> Option<&dyn Node> {
        None
    }
    fn next_sibling(&self) -> Option<&dyn Node> {
        None
    }
}
