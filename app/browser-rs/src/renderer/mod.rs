pub mod css;
pub mod html;
pub mod js;
pub mod layout;

use crate::gui::ApplicationWindow;
use crate::renderer::css::cssom::*;
use crate::renderer::css::token::*;
use crate::renderer::html::dom::*;
use crate::renderer::html::token::*;
use crate::renderer::layout::render_tree::*;
use alloc::rc::Rc;
use alloc::string::String;
use core::cell::RefCell;
use liumlib::*;
use saji::ast::Parser;
use saji::runtime::Runtime;
use saji::token::Lexer;

pub fn render(html: String, window: &ApplicationWindow) {
    //println!("Input HTML:\n{}", html);
    //println!("----------------------");

    // html
    let html_tokenizer = HtmlTokenizer::new(html);
    let dom_root = HtmlParser::new(html_tokenizer).construct_tree();
    //println!("DOM:");
    print_dom(&Some(dom_root.clone()), 0);
    //println!("----------------------");

    // css
    let style = get_style_content(dom_root.clone());
    let css_tokenizer = CssTokenizer::new(style);
    let cssom = CssParser::new(css_tokenizer).parse_stylesheet();

    //println!("CSSOM:\n{:?}", cssom);
    //println!("----------------------");

    // js
    let js = get_js_content(dom_root.clone());
    let lexer = Lexer::new(js);
    //println!("JS lexer {:?}", lexer);

    let mut parser = Parser::new(lexer);
    let ast = parser.parse_ast();
    //println!("JS ast {:?}", ast);

    let mut runtime = Runtime::new();
    runtime.execute(&ast);

    // apply css to html and create RenderTree
    let render_tree = RenderTree::new(dom_root, &cssom);

    println!("----------------------");
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
