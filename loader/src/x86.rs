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
