use core::fmt;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct CStrPtr16 {
    ptr: *const u16,
}
impl CStrPtr16 {
    pub fn from_ptr(p: *const u16) -> CStrPtr16 {
        CStrPtr16 { ptr: p }
    }
}

pub fn strlen_char16(strp: CStrPtr16) -> usize {
    let mut len: usize = 0;
    unsafe {
        loop {
            if *strp.ptr.add(len) == 0 {
                break;
            }
            len += 1;
        }
    }
    len
}

impl fmt::Display for CStrPtr16 {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        unsafe {
            let mut index = 0;
            loop {
                let c = *self.ptr.offset(index);
                if c == 0 {
                    break;
                }
                let bytes = c.to_be_bytes();
                let result = write!(f, "{}", bytes[1] as char);
                if result.is_err() {
                    return result;
                }
                index += 1;
            }
        }
        Result::Ok(())
    }
}

#[repr(C)]
pub struct EFITableHeader {
    pub signature: u64,
    pub revision: u32,
    pub header_size: u32,
    pub crc32: u32,
    reserved: u32,
}

#[repr(C)]
pub struct EFI_GUID {
    data0: u32,
    data1: u16,
    data2: u16,
    data3: [u8; 8],
}

pub type EFIHandle = u64;
pub type EFIVoid = u8;
pub type EFINativeUInt = u64;

pub const EFI_SYSTEM_TABLE_SIGNATURE: u64 = 0x5453595320494249;
pub const EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x9042a9de,
    data1: 0x23dc,
    data2: 0x4a38,
    data3: [0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a],
};
pub const EFI_LOADED_IMAGE_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x5B1B31A1,
    data1: 0x9562,
    data2: 0x11d2,
    data3: [0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B],
};
pub const EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID: EFI_GUID = EFI_GUID {
    data0: 0x0964e5b22,
    data1: 0x6459,
    data2: 0x11d2,
    data3: [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
};
pub const EFI_FILE_SYSTEM_INFO_GUID: EFI_GUID = EFI_GUID {
    data0: 0x09576e93,
    data1: 0x6d3f,
    data2: 0x11d2,
    data3: [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
};
pub const MEMORY_MAP_BUFFER_SIZE: usize = 0x8000;

#[repr(i64)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EFIStatus {
    SUCCESS = 0,
}

#[repr(C)]
pub struct EFISimpleTextOutputProtocol {
    pub reset: EFIHandle,
    pub output_string:
        extern "win64" fn(this: *const EFISimpleTextOutputProtocol, str: *const u16) -> EFIStatus,
    pub test_string: EFIHandle,
    pub query_mode: EFIHandle,
    pub set_mode: EFIHandle,
    pub set_attribute: EFIHandle,
    pub clear_screen: extern "win64" fn(this: *const EFISimpleTextOutputProtocol) -> EFIStatus,
}

pub union EFIBootServicesTableLocateProtocolVariants {
    pub locate_graphics_output_protocol: extern "win64" fn(
        protocol: *const EFI_GUID,
        registration: *const EFIVoid,
        interface: *mut *mut EFIGraphicsOutputProtocol,
    ) -> EFIStatus,
}
pub union EFIBootServicesTableHandleProtocolVariants {
    pub handle_loaded_image_protocol: extern "win64" fn(
        handle: EFIHandle,
        protocol: *const EFI_GUID,
        interface: *mut *mut EFILoadedImageProtocol,
    ) -> EFIStatus,
    pub handle_simple_file_system_protocol: extern "win64" fn(
        handle: EFIHandle,
        protocol: *const EFI_GUID,
        interface: *mut *mut EFISimpleFileSystemProtocol,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFIBootServicesTable {
    pub header: EFITableHeader,
    padding0: [u64; 4],
    pub get_memory_map: extern "win64" fn(
        memory_map_size: *mut EFINativeUInt,
        memory_map: *mut u8,
        map_key: *mut EFINativeUInt,
        descriptor_size: *mut EFINativeUInt,
        descriptor_version: *mut u32,
    ) -> EFIStatus,
    padding1: [u64; 11],
    pub handle_protocol: EFIBootServicesTableHandleProtocolVariants,
    padding2: [u64; 9],
    pub exit_boot_services:
        extern "win64" fn(image_handle: EFIHandle, map_key: EFINativeUInt) -> EFIStatus,
    padding3: [u64; 5],
    pub open_protocol: EFIHandle,
    padding4: [u64; 4],
    pub locate_protocol: EFIBootServicesTableLocateProtocolVariants,
}

#[repr(C)]
pub struct EFISystemTable<'a> {
    pub header: EFITableHeader,
    pub firmware_vendor: CStrPtr16,
    pub firmware_revision: u32,
    pub console_in_handle: EFIHandle,
    pub con_in: EFIHandle,
    pub console_out_handle: EFIHandle,
    pub con_out: &'a EFISimpleTextOutputProtocol,
    pub standard_error_handle: EFIHandle,
    pub std_err: &'a EFISimpleTextOutputProtocol,
    pub runtime_services: EFIHandle,
    pub boot_services: &'a EFIBootServicesTable,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocolPixelInfo {
    pub version: u32,
    pub horizontal_resolution: u32,
    pub vertical_resolution: u32,
    pub pixel_format: u32,
    pub red_mask: u32,
    pub green_mask: u32,
    pub blue_mask: u32,
    pub reserved_mask: u32,
    pub pixels_per_scan_line: u32,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocolMode<'a> {
    pub max_mode: u32,
    pub mode: u32,
    pub info: &'a EFIGraphicsOutputProtocolPixelInfo,
    pub size_of_info: u64,
    pub frame_buffer_base: u64,
    pub frame_buffer_size: u64,
}

#[repr(C)]
#[derive(Debug)]
pub struct EFIGraphicsOutputProtocol<'a> {
    reserved: [u64; 3],
    pub mode: &'a EFIGraphicsOutputProtocolMode<'a>,
}

#[repr(C)]
#[derive(Default)]
pub struct EFIFileSystemInfo {
    pub size: u64,
    pub readonly: u8,
    pub volume_size: u64,
    pub free_space: u64,
    pub block_size: u32,
    pub volume_label: [u16; 32],
}

#[repr(C)]
pub struct EFIFileProtocol {
    pub revision: u64,
    reserved0: [u64; 3],
    pub read: extern "win64" fn(
        this: *const EFIFileProtocol,
        buffer_size: &mut u64,
        buffer: &mut EFIFileInfo,
    ) -> EFIStatus,
    reserved1: [u64; 3],
    pub get_info: extern "win64" fn(
        this: *const EFIFileProtocol,
        information_type: *const EFI_GUID,
        buffer_size: &mut u64,
        buffer: &mut EFIFileSystemInfo,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFILoadedImageProtocol<'a> {
    pub revision: u32,
    pub parent_handle: EFIHandle,
    pub system_table: &'a EFISystemTable<'a>,
    pub device_handle: EFIHandle,
}

#[allow(dead_code)]
#[derive(Default)]
pub struct EFITime {
    year: u16,  // 1900 – 9999
    month: u8,  // 1 – 12
    day: u8,    // 1 – 31
    hour: u8,   // 0 – 23
    minute: u8, // 0 – 59
    second: u8, // 0 – 59
    pad1: u8,
    nanosecond: u32, // 0 – 999,999,999
    time_zone: u16,  // -1440 to 1440 or 2047
    daylight: u8,
    pad2: u8,
}

#[repr(C)]
#[derive(Default)]
pub struct EFIFileInfo {
    pub size: u64,
    pub file_size: u64,
    pub physical_size: u64,
    pub create_time: EFITime,
    pub last_access_time: EFITime,
    pub modification_time: EFITime,
    pub attr: u64,
    pub file_name: [u16; 32],
}

#[repr(C)]
pub struct FileProtocol {
    revision: u64,
    open: EFIHandle,
    close: EFIHandle,
    delete: EFIHandle,
    read: extern "win64" fn(
        this: *const EFISimpleFileSystemProtocol,
        buffer_size: &mut u64,
        buffer: &mut EFIFileInfo,
    ) -> EFIStatus,
}

#[repr(C)]
pub struct EFISimpleFileSystemProtocol {
    pub revision: u64,
    pub open_volume: extern "win64" fn(
        this: *const EFISimpleFileSystemProtocol,
        root: *mut *mut EFIFileProtocol,
    ) -> EFIStatus,
}

pub struct EFISimpleTextOutputProtocolWriter<'a> {
    pub protocol: &'a EFISimpleTextOutputProtocol,
}

impl EFISimpleTextOutputProtocolWriter<'_> {
    pub fn write_char(&mut self, c: u8) {
        let cbuf: [u16; 2] = [c.into(), 0];
        (self.protocol.output_string)(self.protocol, cbuf.as_ptr());
    }
    pub fn write_str(&mut self, s: &str) {
        for c in s.bytes() {
            if c == b'\n' {
                self.write_char(b'\r');
            }
            self.write_char(c);
        }
    }
}

impl fmt::Write for EFISimpleTextOutputProtocolWriter<'_> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write_str(s);
        Ok(())
    }
}
