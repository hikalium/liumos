pub mod css;
pub mod dom;
pub mod tokenizer;

use crate::gui::ApplicationWindow;
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec::Vec;
use core::cell::RefCell;
use css::*;
use dom::*;
use liumlib::*;
use tokenizer::*;

pub fn render(html: String, _window: &ApplicationWindow) -> Vec<Rule> {
    println!("input html:\n{}", html);

    let t = Tokenizer::new(html);
    let root = Parser::new(t).construct_tree();
    print_node(Some(root.clone()), 0);

    let style = get_style_content(root.clone());
    let style_content = style.unwrap();

    let t2 = CssTokenizer::new(style_content);
    let mut p2 = CssParser::new(t2);
    let rules = p2.construct_tree();

    rules
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
