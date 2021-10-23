pub mod css_token;
pub mod cssom;
pub mod dom;
pub mod html_token;

use crate::gui::ApplicationWindow;
use crate::renderer::css_token::*;
use crate::renderer::cssom::*;
use crate::renderer::dom::*;
use crate::renderer::html_token::*;
use alloc::rc::Rc;
use alloc::string::String;
use alloc::vec::Vec;
use core::cell::RefCell;
use liumlib::*;

pub fn render(html: String, _window: &ApplicationWindow) -> Vec<CssRule> {
    println!("input html:\n{}", html);

    let t = HtmlTokenizer::new(html);
    let root = HtmlParser::new(t).construct_tree();
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
