use core::fmt;
use core::mem;

pub struct RegionHeader<'a> {
    number_of_pages: usize,
    number_of_pages_used: usize,
    next_region: Option<&'a mut RegionHeader<'a>>,
}
impl<'a> RegionHeader<'a> {
    pub fn init(&mut self, number_of_pages: usize) {
        self.number_of_pages = number_of_pages;
        self.next_region = None;
        let header_size_with_bitmap = mem::size_of::<Self>() + number_of_pages / 8;
        self.number_of_pages_used = (header_size_with_bitmap + 0xFFF) >> 12;
        for index in 0..self.number_of_pages_used {
            // Mark header and bitmap pages as used
            self.write_allocation_bitmap(index, true);
        }
    }
    pub fn set_next(&mut self, next: &'a mut RegionHeader<'a>) {
        assert!(self.next_region.is_none());
        self.next_region = Some(next);
    }
    pub fn iter(&'a self) -> RegionHeaderIterator {
        RegionHeaderIterator {
            current: Some(&self),
        }
    }
    fn read_allocation_bitmap(&mut self, index: usize) -> bool {
        let bitmap = ((self as *mut Self as usize) + mem::size_of::<Self>()) as *mut u8;
        let byte_index = index / 8;
        let bit_index = index % 8;
        unsafe { *bitmap.add(byte_index) & (1 << bit_index) != 0 }
    }
    fn write_allocation_bitmap(&mut self, index: usize, data: bool) {
        let bitmap = ((self as *mut Self as usize) + mem::size_of::<Self>()) as *mut u8;
        let byte_index = index / 8;
        let bit_index = index % 8;
        if data {
            unsafe {
                *bitmap.add(byte_index) |= 1 << bit_index;
            }
        } else {
            unsafe {
                *bitmap.add(byte_index) &= !(1 << bit_index);
            }
        }
    }
    pub fn allocate_physical_page(&mut self) -> Option<usize> {
        if self.number_of_pages_used == self.number_of_pages {
            match self.next_region {
                Some(ref mut next) => next.allocate_physical_page(),
                None => None,
            }
        } else {
            let mut free_index: Option<usize> = None;
            for index in 0..self.number_of_pages {
                if !self.read_allocation_bitmap(index) {
                    self.write_allocation_bitmap(index, true);
                    free_index = Some(index);
                    break;
                }
            }
            assert!(free_index.is_some());
            let allocated_page_addr = (self as *const Self as usize) + (free_index.unwrap() << 12);
            self.number_of_pages_used += 1;
            Some(allocated_page_addr)
        }
    }
    pub fn free_physical_page(&mut self, physical_page_addr: usize) {
        assert!(physical_page_addr & 0xFFF == 0);
        let index = (physical_page_addr - (self as *const Self as usize)) >> 12;
        assert!(self.read_allocation_bitmap(index));
        self.write_allocation_bitmap(index, false);
        self.number_of_pages_used -= 1;
    }
}
impl<'a> fmt::Debug for RegionHeader<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "RegionHeader {{ ")?;
        write!(
            f,
            "phys: [{:#018X}-{:#018X}), pages: {}, used_pages: {}",
            self as *const Self as usize,
            self as *const Self as usize + self.number_of_pages * 4096,
            self.number_of_pages,
            self.number_of_pages_used,
        )?;
        write!(f, " }}")
    }
}
#[derive(Debug)]
pub struct RegionHeaderIterator<'a> {
    current: Option<&'a RegionHeader<'a>>,
}
impl<'a> Iterator for RegionHeaderIterator<'a> {
    type Item = &'a RegionHeader<'a>;
    fn next(&mut self) -> Option<&'a RegionHeader<'a>> {
        match self.current {
            Some(current) => {
                self.current = current.next_region.as_ref().map(|x| x as _);
                Some(current)
            }
            None => None,
        }
    }
}
