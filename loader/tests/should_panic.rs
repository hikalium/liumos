#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![reexport_test_harness_main = "test_main"]

use core::panic::PanicInfo;
use loader::debug_exit;
use loader::efi;
use loader::serial;

#[no_mangle]
pub extern "win64" fn efi_main(
    _image_handle: efi::EFIHandle,
    _efi_system_table: &efi::EFISystemTable,
) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "Running a test in {}.", file!()).unwrap();
    assert_eq!(0, 1);
    write!(writer, "FAIL: did not panic on an assertion failure.").unwrap();
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Fail);
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "{}", info).unwrap();
    writeln!(writer, "PASS: panicked as expected.").unwrap();
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Success);
}
