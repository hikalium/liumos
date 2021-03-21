#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![reexport_test_harness_main = "test_main"]
#![test_runner(crate::test_runner)]

use core::panic::PanicInfo;
use loader::debug_exit;
use loader::efi;
use loader::serial;

#[test_case]
fn trivial_assertion() {
    assert_eq!(1, 1);
}

#[no_mangle]
pub extern "win64" fn efi_main(
    _image_handle: efi::EFIHandle,
    _efi_system_table: &efi::EFISystemTable,
) -> ! {
    // Setup prerequisites

    // Run tests
    test_main();
    unreachable!();
}

//
// Test harness implementaion
//

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "{}", info).unwrap();
    write!(writer, "FAIL: did not panic on an assertion failure.").unwrap();
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Fail);
}

fn test_runner(tests: &[&dyn Testable]) -> ! {
    use core::fmt::Write;
    serial::com_initialize(serial::IO_ADDR_COM2);
    let mut writer = serial::SerialConsoleWriter {};
    writeln!(writer, "Running {} tests in {}...", tests.len(), file!()).unwrap();
    for test in tests {
        test.run();
    }
    write!(writer, "Done!").unwrap();
    debug_exit::exit_qemu(debug_exit::QemuExitCode::Success)
}

pub trait Testable {
    fn run(&self);
}

impl<T> Testable for T
where
    T: Fn(),
{
    fn run(&self) {
        use core::fmt::Write;
        serial::com_initialize(serial::IO_ADDR_COM2);
        let mut writer = serial::SerialConsoleWriter {};
        write!(writer, "{}...\t", core::any::type_name::<T>()).unwrap();
        self();
        writeln!(writer, "[PASS]").unwrap();
    }
}
