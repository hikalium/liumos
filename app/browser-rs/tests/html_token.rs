#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(crate::test_runner)]
#![reexport_test_harness_main = "test_main"]

extern crate alloc;

use crate::alloc::string::ToString;
use alloc::string::String;
use alloc::vec::Vec;

use browser_rs::renderer::html::token::{HtmlToken, HtmlTokenizer};
use browser_rs::renderer::html::Attribute;
use liumlib::*;

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
    println!("Running {} tests in tokenizer.rs", tests.len());
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

#[macro_export]
macro_rules! run_test {
    ($html:literal) => {
        let mut t = HtmlTokenizer::new(String::from($html));
        assert_eq!(t.next(), None);
    };
    ($html:literal, $( $token:expr ),*) => {
        let mut t = HtmlTokenizer::new(String::from($html));

        let mut expected = Vec::new();
        $(
            expected.push($token);
        )*

        for e in expected {
            let token = t.next().expect("tokenizer should have a next Token");
            assert_eq!(token, e, "\n\nexpected {:?} but got {:?}", e, token);
        }
    };
}

#[test_case]
fn no_input() {
    run_test!("");
}

#[test_case]
fn chars() {
    run_test!(
        "foo",
        HtmlToken::Char('f'),
        HtmlToken::Char('o'),
        HtmlToken::Char('o')
    );
}

#[test_case]
fn body() {
    run_test!(
        "<body></body>",
        HtmlToken::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::EndTag {
            tag: String::from("body"),
            self_closing: false,
        }
    );
}

#[test_case]
fn BODY() {
    run_test!(
        "<BODY></BODY>",
        HtmlToken::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::EndTag {
            tag: String::from("body"),
            self_closing: false
        }
    );
}

#[test_case]
fn br() {
    run_test!(
        "<br/>",
        HtmlToken::StartTag {
            tag: String::from("br"),
            self_closing: true,
            attributes: Vec::new(),
        }
    );
}

#[test_case]
fn simple_page() {
    run_test!(
        "<html><body>abc</body></html>",
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('a'),
        HtmlToken::Char('b'),
        HtmlToken::Char('c'),
        HtmlToken::EndTag {
            tag: String::from("body"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn div() {
    run_test!(
        "<div>abc</div>",
        HtmlToken::StartTag {
            tag: String::from("div"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('a'),
        HtmlToken::Char('b'),
        HtmlToken::Char('c'),
        HtmlToken::EndTag {
            tag: String::from("div"),
            self_closing: false,
        }
    );
}

#[test_case]
fn link() {
    let mut attributes = Vec::new();
    let mut a1 = Attribute::new();
    a1.set_name_and_value("rel".to_string(), "stylesheet".to_string());
    attributes.push(a1);
    let mut a2 = Attribute::new();
    a2.set_name_and_value("href".to_string(), "styles.css".to_string());
    attributes.push(a2);

    run_test!(
        "<html><head><link rel='stylesheet' href=\"styles.css\"></head></html>",
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("head"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("link"),
            self_closing: false,
            attributes,
        },
        HtmlToken::EndTag {
            tag: String::from("head"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn link_with_unquoted_attribute() {
    let mut attributes = Vec::new();
    let mut a = Attribute::new();
    a.set_name_and_value("href".to_string(), "styles.css".to_string());
    attributes.push(a);

    run_test!(
        "<html><head><link href=styles.css></head></html>",
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("head"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("link"),
            self_closing: false,
            attributes,
        },
        HtmlToken::EndTag {
            tag: String::from("head"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn css_with_style() {
    run_test!(
        "<html><head><style>h1{background-color:red;}</style></head></html>",
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("head"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("style"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('h'),
        HtmlToken::Char('1'),
        HtmlToken::Char('{'),
        HtmlToken::Char('b'),
        HtmlToken::Char('a'),
        HtmlToken::Char('c'),
        HtmlToken::Char('k'),
        HtmlToken::Char('g'),
        HtmlToken::Char('r'),
        HtmlToken::Char('o'),
        HtmlToken::Char('u'),
        HtmlToken::Char('n'),
        HtmlToken::Char('d'),
        HtmlToken::Char('-'),
        HtmlToken::Char('c'),
        HtmlToken::Char('o'),
        HtmlToken::Char('l'),
        HtmlToken::Char('o'),
        HtmlToken::Char('r'),
        HtmlToken::Char(':'),
        HtmlToken::Char('r'),
        HtmlToken::Char('e'),
        HtmlToken::Char('d'),
        HtmlToken::Char(';'),
        HtmlToken::Char('}'),
        HtmlToken::EndTag {
            tag: String::from("style"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("head"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn format() {
    run_test!(
        r#"<html>
  <body>
    Hello
    <ul>
        <li>list 1</li>
        <li>list 2</li>
    </ul>
  </body>
</html>"#,
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::StartTag {
            tag: String::from("body"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char('H'),
        HtmlToken::Char('e'),
        HtmlToken::Char('l'),
        HtmlToken::Char('l'),
        HtmlToken::Char('o'),
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::StartTag {
            tag: String::from("ul"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::StartTag {
            tag: String::from("li"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('l'),
        HtmlToken::Char('i'),
        HtmlToken::Char('s'),
        HtmlToken::Char('t'),
        HtmlToken::Char(' '),
        HtmlToken::Char('1'),
        HtmlToken::EndTag {
            tag: String::from("li"),
            self_closing: false,
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::StartTag {
            tag: String::from("li"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('l'),
        HtmlToken::Char('i'),
        HtmlToken::Char('s'),
        HtmlToken::Char('t'),
        HtmlToken::Char(' '),
        HtmlToken::Char('2'),
        HtmlToken::EndTag {
            tag: String::from("li"),
            self_closing: false,
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::EndTag {
            tag: String::from("ul"),
            self_closing: false,
        },
        HtmlToken::Char('\n'),
        HtmlToken::Char(' '),
        HtmlToken::Char(' '),
        HtmlToken::EndTag {
            tag: String::from("body"),
            self_closing: false,
        },
        HtmlToken::Char('\n'),
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}

#[test_case]
fn js_with_style() {
    run_test!(
        "<html><head><script>1+2</script></head></html>",
        HtmlToken::StartTag {
            tag: String::from("html"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("head"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::StartTag {
            tag: String::from("script"),
            self_closing: false,
            attributes: Vec::new(),
        },
        HtmlToken::Char('1'),
        HtmlToken::Char('+'),
        HtmlToken::Char('2'),
        HtmlToken::EndTag {
            tag: String::from("script"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("head"),
            self_closing: false,
        },
        HtmlToken::EndTag {
            tag: String::from("html"),
            self_closing: false,
        }
    );
}
