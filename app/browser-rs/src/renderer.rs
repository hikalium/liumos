pub mod css_token;
pub mod cssom;
pub mod dom;
pub mod html_token;
pub mod render_tree;

use crate::gui::ApplicationWindow;
use crate::renderer::css_token::*;
use crate::renderer::cssom::*;
use crate::renderer::dom::*;
use crate::renderer::html_token::*;
use crate::renderer::render_tree::*;
use alloc::rc::Rc;
use alloc::string::String;
use core::cell::RefCell;
use liumlib::*;

pub fn render(html: String, window: &ApplicationWindow) {
    println!("Input HTML:\n{}", html);
    println!("----------------------");

    let html_tokenizer = HtmlTokenizer::new(html);
    let dom_root = HtmlParser::new(html_tokenizer).construct_tree();
    println!("DOM:");
    print_dom(&Some(dom_root.clone()), 0);
    println!("----------------------");

    let style = get_style_content(dom_root.clone());
    let style_content = style.unwrap();

    let css_tokenizer = CssTokenizer::new(style_content);
    let cssom = CssParser::new(css_tokenizer).parse_stylesheet();

    println!("CSSOM:\n{:?}", cssom);
    println!("----------------------");

    let mut render_tree = RenderTree::new(dom_root);
    render_tree.apply(&cssom);

    println!("Render Tree:");
    print_render_object(&render_tree.root, 0);
    println!("----------------------");

    render_tree.paint(window);
}

fn print_dom(node: &Option<Rc<RefCell<Node>>>, depth: usize) {
    match node {
        Some(n) => {
            print!("{}", "  ".repeat(depth));
            println!("{:?}", n.borrow().kind);
            print_dom(&n.borrow().first_child(), depth + 1);
            print_dom(&n.borrow().next_sibling(), depth);
        }
        None => return,
    }
}

fn print_render_object(node: &Option<Rc<RefCell<RenderObject>>>, depth: usize) {
    match node {
        Some(n) => {
            print!("{}", "  ".repeat(depth));
            println!("{:?} {:?}", n.borrow().kind, n.borrow().style);
            print_render_object(&n.borrow().first_child(), depth + 1);
            print_render_object(&n.borrow().next_sibling(), depth);
        }
        None => return,
    }
}
