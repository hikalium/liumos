//! This is a part of in the "CSS Box Model Module Level 3" spec.
//! https://www.w3.org/TR/css-box-3/

use crate::alloc::string::ToString;
use alloc::string::String;

#[allow(dead_code)]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BoxInfo {
    // true for block boxes and false for inline boxes
    is_block_type: bool,
    // true for inner display type and false for outer display type
    is_inner_display_type: bool,
    width: String,
    height: String,
    // top, right, bottom, left
    margin: (usize, usize, usize, usize),
    // top, right, bottom, left
    padding: (usize, usize, usize, usize),
    // witdh, style, color
    border: (String, String, String),
}

#[allow(dead_code)]
impl BoxInfo {
    pub fn new() -> Self {
        Self {
            is_block_type: true,
            is_inner_display_type: true,
            width: "auto".to_string(),
            height: "auto".to_string(),
            margin: (0, 0, 0, 0),
            padding: (0, 0, 0, 0),
            border: (
                "medium".to_string(),
                "none".to_string(),
                "color".to_string(),
            ),
        }
    }
}
