pub mod css;
pub mod dom;
pub mod tokenizer;

use crate::gui::ApplicationWindow;
use alloc::rc::Rc;
use alloc::string::String;
use core::cell::RefCell;
use dom::*;
use liumlib::*;
use tokenizer::*;

pub fn render(html: String, _window: &ApplicationWindow) {
    println!("input html:\n{}", html);

    let t = Tokenizer::new(html);
    let root = Parser::new(t).construct_tree();
    print_node(Some(root.clone()), 0);
}

fn print_node(node: Option<Rc<RefCell<Node>>>, depth: usize) {
    match node {
        Some(n) => {
            print!("{}", "  ".repeat(depth));
            println!("{:?}", n.borrow().kind);
            print_node(n.borrow().first_child(), depth + 1);
            print_node(n.borrow().next_sibling(), depth);
        }
        None => return,
    }
}
