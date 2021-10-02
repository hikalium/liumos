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

use browser_rs::renderer::dom::*;
use browser_rs::renderer::tokenizer::*;
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

#[macro_export]
macro_rules! run_test {
    ($html:literal, $expected_root:expr) => {
        use browser_rs::renderer::dom::*;

        let t = Tokenizer::new(String::from($html));

        let mut p = Parser::new(t);
        let root_raw = p.construct_tree();
        let root = Some(root_raw.clone());
        println!("\n----- nodes -----");
        print_node(Some(root_raw.clone()), 0);

        // Check nodes recursively.
        assert!(node_equals($expected_root, root.clone()));
    };
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
    let body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    let text = Rc::new(RefCell::new(Node::new(NodeKind::Text(String::from("foo")))));
    // body <--> text
    {
        body.borrow_mut().first_child = Some(text.clone());
    }
    {
        text.borrow_mut().parent = Some(Rc::downgrade(&body));
    }

    run_test!("<html><head></head><body>foo</body></html>", Some(root));
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
    let body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    let ul = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Ul,
    )))));
    let li1 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));
    let li2 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));

    // body <--> ul
    {
        body.borrow_mut().first_child = Some(ul.clone());
    }
    {
        ul.borrow_mut().parent = Some(Rc::downgrade(&body));
    }

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
    let body = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .next_sibling()
        .unwrap();
    let ul1 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Ul,
    )))));
    let li1 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));
    let li2 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));

    let ul2 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Ul,
    )))));
    let li3 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));
    let li4 = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(
        ElementKind::Li,
    )))));

    // body <--> ul1
    {
        body.borrow_mut().first_child = Some(ul1.clone());
    }
    {
        ul1.borrow_mut().parent = Some(Rc::downgrade(&body));
    }

    // ul1 <--> li1
    {
        ul1.borrow_mut().first_child = Some(li1.clone());
    }
    {
        li1.borrow_mut().parent = Some(Rc::downgrade(&ul1));
    }

    // li1 <--> li2
    {
        li1.borrow_mut().next_sibling = Some(li2.clone());
    }
    {
        li2.borrow_mut().previous_sibling = Some(Rc::downgrade(&li1));
    }

    // ul1 <--> ul2
    {
        ul1.borrow_mut().next_sibling = Some(ul2.clone());
    }
    {
        ul2.borrow_mut().previous_sibling = Some(Rc::downgrade(&ul1));
    }

    // ul2 <--> li3
    {
        ul2.borrow_mut().first_child = Some(li3.clone());
    }
    {
        li3.borrow_mut().parent = Some(Rc::downgrade(&ul2));
    }

    // li3 <--> li4
    {
        li3.borrow_mut().next_sibling = Some(li4.clone());
    }
    {
        li4.borrow_mut().previous_sibling = Some(Rc::downgrade(&li3));
    }

    run_test!("<html><head></head><body><ul><li></li><li></li></ul><ul><li></li><li></li></ul></body></html>", Some(root));
}

#[test_case]
fn list() {
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
    let head = root
        .borrow_mut()
        .first_child()
        .unwrap()
        .borrow_mut()
        .first_child()
        .unwrap();
    let style = Rc::new(RefCell::new(Node::new(NodeKind::Element(Element::new(ElementKind::Script)))));

    // head <--> style
    {
        head.borrow_mut().first_child = Some(style.clone());
    }
    {
        style.borrow_mut().parent = Some(Rc::downgrade(&head));
    }

    run_test!(
        "<html><head><style>h1{background-color:red;}</style></head></html>",
        Some(root)
    );
}
