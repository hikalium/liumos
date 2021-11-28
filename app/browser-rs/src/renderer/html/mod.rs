pub mod dom;
pub mod html_token;

use alloc::string::String;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Attribute {
    pub name: String,
    pub value: String,
}

impl Attribute {
    pub fn new() -> Self {
        Self {
            name: String::new(),
            value: String::new(),
        }
    }

    fn set_name(&mut self, name: String) {
        self.name = name;
    }

    fn set_value(&mut self, value: String) {
        self.value = value;
    }

    fn add_char(&mut self, c: char, is_name: bool) {
        if is_name {
            self.name.push(c);
        } else {
            self.value.push(c);
        }
    }

    // This is for test.
    #[allow(dead_code)]
    pub fn set_name_and_value(&mut self, name: String, value: String) {
        self.name = name;
        self.value = value;
    }
}

