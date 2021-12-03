#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::rc::Rc;
use alloc::string::String;
use core::cell::RefCell;

use browser_rs::renderer::html::dom::{Element, ElementKind, HtmlParser, Node, NodeKind};
use browser_rs::renderer::html::token::HtmlTokenizer;
use liumlib::*;

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

#[cfg(test)]
pub trait Testable {
    fn run(&self) -> ();
}

#[cfg(test)]
impl<T> Testable for T
where
    T: Fn(),
{
    fn run(&self) {
        print!("{} ...\t", core::any::type_name::<T>());
        self();
        println!("[ok]");
    }
}

#[cfg(test)]
pub fn test_runner(tests: &[&dyn Testable]) {
    println!("Running {} tests in dom.rs", tests.len());
    for test in tests {
        test.run();
    }
}

#[macro_export]
macro_rules! run_test {
    ($html:literal, $expected_root:expr) => {
        let t = HtmlTokenizer::new(String::from($html));

        let mut p = HtmlParser::new(t);
        let root_raw = p.construct_tree();
        let root = Some(root_raw.clone());
        println!("\n----- nodes -----");
        print_node(Some(root_raw.clone()), 0);

        // Check nodes recursively.
        assert!(node_equals($expected_root, root.clone()));
    };
}

#[cfg(test)]
entry_point!(main);
#[cfg(test)]
fn main() {
    test_main();
}

/// Checks the node kinds recursively.
fn node_equals(expected: Option<Rc<RefCell<Node>>>, actual: Option<Rc<RefCell<Node>>>) -> bool {
    if expected.is_some() != actual.is_some() {
        // A node is Some and another is None.
        println!("expected {:?} but actual {:?}", expected, actual);
        return false;
    }

    if expected.is_none() {
        // Both nodes are None, return true.
        return true;
    }

    // Both nodes are Some, check kind.
    let expected_node = expected.unwrap();
    let expected_borrowed = expected_node.borrow();
    let actual_node = actual.unwrap();
    let actual_borrowed = actual_node.borrow();
    if expected_borrowed.kind != actual_borrowed.kind {
        println!(
            "expected {:?} but actual {:?}",
            expected_borrowed, actual_borrowed
        );
        return false;
    }

    node_equals(
        expected_borrowed.first_child().clone(),
        actual_borrowed.first_child().clone(),
    ) && node_equals(
        expected_borrowed.next_sibling().clone(),
        actual_borrowed.next_sibling().clone(),
    )
}

fn create_base_dom_tree() -> Rc<RefCell<Node>> {
    // root (Document)
    // └── html
    //     └── head
    //     └── body

    let root = Rc::new(RefCell::new(Node::new(NodeKind::Document)));
    let html = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Html,
    )))));
    let head = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Head,
    )))));
    let body = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Body,
    )))));

    // root <--> html
    {
        html.borrow_mut().parent = Some(Rc::downgrade(&root));
    }
    {
        root.borrow_mut().first_child = Some(html.clone());
    }
    {
        root.borrow_mut().last_child = Some(Rc::downgrade(&html));
    }

    // html <--> head
    {
        head.borrow_mut().parent = Some(Rc::downgrade(&html));
    }
    {
        html.borrow_mut().first_child = Some(head.clone());
    }
    {
        html.borrow_mut().last_child = Some(Rc::downgrade(&head));
    }

    // html <--> body
    {
        body.borrow_mut().parent = Some(Rc::downgrade(&html));
    }
    {
        html.borrow_mut()
            .first_child()
            .unwrap()
            .borrow_mut()
            .next_sibling = Some(body.clone());
    }
    {
        body.borrow_mut().previous_sibling = Some(Rc::downgrade(&html));
    }

    root.clone()
}

fn add_text_node_to(target: &mut Rc<RefCell<Node>>, s: &str) {
    //  └── target
    //      └── child
    let text = Rc::new(RefCell::new(Node::new(NodeKind::Text(String::from(s)))));
    add_child_node_to(target, &text);
}

fn add_child_node_to(target: &mut Rc<RefCell<Node>>, child: &Rc<RefCell<Node>>) {
    //  └── target
    //      └── child
    {
        target.borrow_mut().first_child = Some(child.clone());
    }
    {
        child.borrow_mut().parent = Some(Rc::downgrade(&target));
    }
}

fn add_child_ul_to(target: &mut Rc<RefCell<Node>>) {
    //  └── target
    //      └── ul
    //          └── li
    //          └── li
    let ul = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Ul,
    )))));
    let li1 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));
    let li2 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));

    // ul <--> li1
    {
        ul.borrow_mut().first_child = Some(li1.clone());
    }
    {
        li1.borrow_mut().parent = Some(Rc::downgrade(&ul));
    }

    // li1 <--> li2
    {
        li1.borrow_mut().next_sibling = Some(li2.clone());
    }
    {
        li2.borrow_mut().previous_sibling = Some(Rc::downgrade(&li1));
    }

    add_child_node_to(target, &ul);
}

fn add_sibling_node_to(target: &mut Rc<RefCell<Node>>, sibling: &Rc<RefCell<Node>>) {
    //  └── target
    //  └── sibling
    {
        target.borrow_mut().next_sibling = Some(sibling.clone());
    }
    {
        sibling.borrow_mut().previous_sibling = Some(Rc::downgrade(&target));
    }
}

fn add_sibling_ul_to(target: &mut Rc<RefCell<Node>>) {
    //  └── target
    //  └── ul
    //      └── li
    //      └── li
    let ul = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Ul,
    )))));
    let li1 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));
    let li2 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));

    // ul <--> li1
    {
        ul.borrow_mut().first_child = Some(li1.clone());
    }
    {
        li1.borrow_mut().parent = Some(Rc::downgrade(&ul));
    }

    // li1 <--> li2
    {
        li1.borrow_mut().next_sibling = Some(li2.clone());
    }
    {
        li2.borrow_mut().previous_sibling = Some(Rc::downgrade(&li1));
    }

    add_sibling_node_to(target, &ul);
}

#[test_case]
fn no_input() {
    let root = Rc::new(RefCell::new(Node::new(NodeKind::Document)));
    run_test!("", Some(root));
}

#[test_case]
fn html() {
    let root = create_base_dom_tree();
    run_test!("<html></html>", Some(root));
}

#[test_case]
fn head() {
    let root = create_base_dom_tree();
    run_test!("<html><head></head></html>", Some(root));
}

#[test_case]
fn body() {
    let root = create_base_dom_tree();
    run_test!("<html><head></head><body></body></html>", Some(root));
}

#[test_case]
fn text() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── text
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_text_node_to(&mut body, "foo");

    run_test!("<html><head></head><body>foo</body></html>", Some(root));
}

#[test_case]
fn div() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── div
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();

    let mut div = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Div,
    )))));
    add_text_node_to(&mut div, "foo");

    add_child_node_to(&mut body, &div);

    run_test!(
        "<html><head></head><body><div>foo</div></body></html>",
        Some(root)
    );
}

#[test_case]
fn list() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── ul
    //             └── li
    //             └── li
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_child_ul_to(&mut body);

    run_test!(
        "<html><head></head><body><ul><li></li><li></li></ul></body></html>",
        Some(root)
    );
}

#[test_case]
fn list2() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── ul
    //             └── li
    //             └── li
    //         └── ul
    //             └── li
    //             └── li
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_child_ul_to(&mut body);

    let mut ul = body.borrow_mut().first_child().unwrap();
    add_sibling_ul_to(&mut ul);

    run_test!("<html><head></head><body><ul><li></li><li></li></ul><ul><li></li><li></li></ul></body></html>", Some(root));
}

#[test_case]
fn list_with_text() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── ul
    //             └── li
    //                 └── list 1
    //             └── li
    //                 └── list 2
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_child_ul_to(&mut body);

    let mut li1 = body
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    add_text_node_to(&mut li1, "list 1");

    let mut l12 = li1.borrow_mut().next_sibling().unwrap();
    add_text_node_to(&mut l12, "list 2");

    run_test!(
        "<html><head></head><body><ul><li>list 1</li><li>list 2</li></ul></body></html>",
        Some(root)
    );
}

#[test_case]
fn link() {
    // root (Document)
    // └── html
    //     └── head
    //         └── link
    //     └── body
    let root = create_base_dom_tree();
    let head = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    let mut link_element = Element::new(ElementKind::Link);
    link_element.set_attribute("rel".to_string(), "stylesheet".to_string());
    link_element.set_attribute("href".to_string(), "styles.css".to_string());

    let link = Rc::new(RefCell::new(Node::new(NodeKind::Element(link_element))));

    // head <--> link
    {
        head.borrow_mut().first_child = Some(link.clone());
    }
    {
        link.borrow_mut().parent = Some(Rc::downgrade(&head));
    }

    run_test!(
        "<html><head><link rel='stylesheet' href=\"styles.css\"></head></html>",
        Some(root)
    );
}

#[test_case]
fn css_with_style() {
    // root (Document)
    // └── html
    //     └── head
    //         └── style
    //     └── body
    let root = create_base_dom_tree();
    let mut head = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    let mut style = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Style,
    )))));
    add_text_node_to(&mut style, "h1{background-color:red;}");
    add_child_node_to(&mut head, &style);

    run_test!(
        "<html><head><style>h1{background-color:red;}</style></head></html>",
        Some(root)
    );
}

#[test_case]
fn formatted_body() {
    // root (Document)
    // └── html
    //     └── head
    //     └── body
    //         └── Hello
    //         └── ul
    //             └── li
    //             └── li
    let root = create_base_dom_tree();
    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_text_node_to(&mut body, "\n  ");

    run_test!(
        "<html>
  <body>
  </body>
</html>",
        Some(root)
    );
}

#[test_case]
fn default_page() {
    // root (Document)
    // └── html
    //     └── head
    //         └── style
    //     └── body
    //         └── ul
    //             └── li
    //             └── li
    let root = create_base_dom_tree();
    let mut head = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    let mut style = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Style,
    )))));
    add_text_node_to(&mut style, " h1 { background-color:red; } ");
    add_child_node_to(&mut head, &style);

    let mut body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    add_child_ul_to(&mut body);

    let mut li1 = body
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    add_text_node_to(&mut li1, "list 1");

    let mut l12 = li1.borrow_mut().next_sibling().unwrap();
    add_text_node_to(&mut l12, "list 2");

    // <html>
    //  <head>
    //    <style>
    //    h1 {
    //        background-color:red;
    //    }
    //    </style>
    //    </head>
    //  <body>
    //    <ul>
    //        <li>list 1</li>
    //        <li>list 2</li>
    //    </ul>
    //  </body>
    //</html>",
    run_test!(
        "<html><head><style> h1 { background-color:red; } </style></head><body><ul><li>list 1</li><li>list 2</li></ul></body></html>",
        Some(root)
    );
}
