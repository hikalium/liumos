use core::fmt;

pub fn write_io_port(port: u16, data: u8) {
    unsafe {
        asm!("out dx, al",
            in("al") data,
            in("dx") port)
    }
}

pub fn read_io_port(port: u16) -> u8 {
    let mut data: u8;
    unsafe {
        asm!("in al, dx",
            out("al") data,
            in("dx") port)
    }
    data
}

pub fn hlt() {
    unsafe { asm!("hlt") }
}

pub fn read_cr3() -> *mut PML4 {
    let mut cr3: *mut PML4;
    unsafe {
        asm!("mov rax, cr3",
            out("rax") cr3)
    }
    cr3
}

#[derive(Debug)]
pub struct PML4Entry {
    value: u64,
}

impl PML4Entry {
    pub fn is_present(&self) -> bool {
        (self.value & 1) != 0
    }
}

impl fmt::Display for PML4Entry {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "PML4E {{ {:#018X} }}", self.value)
    }
}

#[derive(Debug)]
pub struct PML4 {
    entry: [PML4Entry; 512],
}

impl fmt::Display for PML4 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        writeln!(f, "PML4 @ {:#p} {{", self)?;
        for i in 0..512 {
            if !self.entry[i].is_present() {
                continue;
            }
            writeln!(f, "  entry[{}] = {}", i, self.entry[i])?;
        }
        writeln!(f, "}}")?;
        Result::Ok(())
    }
}
