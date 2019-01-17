#pragma once

#include "generic.h"
#include "guid.h"

namespace EFI {

typedef uint64_t UINTN;
typedef void* Handle;

enum class Status : UINTN {
  kSuccess,
};

typedef enum {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown,
  EfiResetPlatformSpecific
} ResetType;

enum class MemoryType : uint32_t {
  kReserved,
  kLoaderCode,
  kLoaderData,
  kBootServicesCode,
  kBootServicesData,
  kRuntimeServicesCode,
  kRuntimeServicesData,
  kConventionalMemory,
  kUnusableMemory,
  kACPIReclaimMemory,
  kACPIMemoryNVS,
  kMemoryMappedIO,
  kMemoryMappedIOPortSpace,
  kPalCode,
  kPersistentMemory,
  kMaxMemoryType
};

typedef enum { TimerCancel, TimerPeriodic, TimerRelative } TimerDelay;

struct ConfigurationTable {
  GUID vendor_guid;
  void* vendor_table;
};

struct TableHeader {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc32;
  uint32_t reserved;
};

struct InputKey {
  uint16_t ScanCode;
  wchar_t UnicodeChar;
};

struct SimpleTextInputProtocol {
  uint64_t _buf;
  uint64_t (*ReadKeyStroke)(SimpleTextInputProtocol*, InputKey*);
  void* wait_for_key;
};

struct SimpleTextOutputProtocol {
  uint64_t _buf;
  uint64_t (*output_string)(SimpleTextOutputProtocol*, const wchar_t*);
  uint64_t (*test_string)(SimpleTextOutputProtocol*, wchar_t*);
  uint64_t (*query_mode)(SimpleTextOutputProtocol*,
                         wchar_t*,
                         uint64_t* columns,
                         uint64_t* rows);
  uint64_t (*set_mode)(SimpleTextOutputProtocol*, uint64_t);
  uint64_t (*set_attribute)(SimpleTextOutputProtocol*, uint64_t Attribute);
  uint64_t (*clear_screen)(SimpleTextOutputProtocol*);
  uint64_t _buf4[2];
  struct SIMPLE_TEXT_OUTPUT_MODE {
    int MaxMode;
    int Mode;
    int Attribute;
    int CursorColumn;
    int CursorRow;
    uint8_t CursorVisible;
  } * Mode;
};

struct Time {
  uint16_t Year;   // 1900 – 9999
  uint8_t Month;   // 1 – 12
  uint8_t Day;     // 1 – 31
  uint8_t Hour;    // 0 – 23
  uint8_t Minute;  // 0 – 59
  uint8_t Second;  // 0 – 59
  uint8_t Pad1;
  uint32_t Nanosecond;  // 0 – 999,999,999
  uint16_t TimeZone;    // -1440 to 1440 or 2047
  uint8_t Daylight;
  uint8_t Pad2;
};

struct FileInfo {
  uint64_t Size;
  uint64_t FileSize;
  uint64_t PhysicalSize;
  Time CreateTime;
  Time LastAccessTime;
  Time ModificationTime;
  uint64_t Attribute;
  wchar_t file_name[1];
};

enum FileProtocolModes : uint64_t {
  kRead = 1,
};

struct FileProtocol {
  uint64_t revision;
  Status (*Open)(FileProtocol* self,
                 FileProtocol** new_handle,
                 const wchar_t* rel_path,
                 FileProtocolModes mode,
                 uint64_t attr);
  Status (*Close)();
  Status (*Delete)();
  Status (*Read)(FileProtocol* self, UINTN* buffer_size, uint8_t* buffer);
  uint64_t (*Write)();
  uint64_t _buf3[2];
  uint64_t (*GetInfo)();
  uint64_t _buf4;
  uint64_t (*Flush)();
};

struct SimpleFileSystemProtocol {
  uint64_t Revision;
  Status (*OpenVolume)(SimpleFileSystemProtocol* self, FileProtocol** Root);
};

struct RuntimeServices {
  char _buf_rs1[24];
  uint64_t _buf_rs2[4];
  uint64_t _buf_rs3[2];
  uint64_t _buf_rs4[3];
  uint64_t _buf_rs5;
  void (*reset_system)(ResetType,
                       uint64_t reset_status,
                       uint64_t data_size,
                       void*);
};

struct MemoryDescriptor {
  MemoryType type;
  uint64_t physical_start;
  uint64_t virtual_start;
  uint64_t number_of_pages;
  uint64_t attribute;

  void Print(void) const;
};

struct DevicePathProtocol {
  unsigned char Type;
  unsigned char SubType;
  unsigned char Length[2];
};

struct BootServices {
  char _buf1[24];
  uint64_t _buf2[2];
  uint64_t _buf3[2];
  Status (*GetMemoryMap)(UINTN* memory_map_size,
                         uint8_t*,
                         UINTN* map_key,
                         UINTN* descriptor_size,
                         uint32_t* descriptor_version);
  uint64_t (*AllocatePool)(MemoryType, uint64_t, void**);
  uint64_t (*FreePool)(void* Buffer);
  uint64_t (*CreateEvent)(unsigned int Type,
                          uint64_t NotifyTpl,
                          void (*NotifyFunction)(void* Event, void* Context),
                          void* NotifyContext,
                          void* Event);
  uint64_t (*SetTimer)(void* Event, TimerDelay, uint64_t TriggerTime);
  uint64_t (*WaitForEvent)(uint64_t NumberOfEvents,
                           void** Event,
                           uint64_t* Index);
  uint64_t _buf4_2[3];
  uint64_t _buf5[9];
  uint64_t (*LoadImage)(unsigned char BootPolicy,
                        void* ParentImageHandle,
                        DevicePathProtocol*,
                        void* SourceBuffer,
                        uint64_t SourceSize,
                        void** ImageHandle);
  uint64_t (*StartImage)(void* ImageHandle,
                         uint64_t* ExitDataSize,
                         unsigned short** ExitData);
  uint64_t _buf6[2];
  Status (*ExitBootServices)(void* image_handle, UINTN map_key);
  uint64_t _buf7[2];
  uint64_t (*SetWatchdogTimer)(uint64_t Timeout,
                               uint64_t WatchdogCode,
                               uint64_t DataSize,
                               unsigned short* WatchdogData);
  uint64_t _buf8[2];
  uint64_t (*OpenProtocol)(void* Handle,
                           GUID* Protocol,
                           void** Interface,
                           void* AgentHandle,
                           void* ControllerHandle,
                           unsigned int Attributes);
  uint64_t _buf9[2];
  uint64_t _buf10[2];
  uint64_t (*LocateProtocol)(const GUID* Protocol,
                             void* Registration,
                             void** Interface);
  uint64_t _buf10_2[2];
  uint64_t _buf11;
  void (*CopyMem)(void* Destination, void* Source, uint64_t Length);
  void (*SetMem)(void* Buffer, uint64_t Size, unsigned char Value);
  uint64_t _buf12;
};

struct SystemTable {
  TableHeader header;
  wchar_t* firmware_vendor;
  uint32_t firmware_revision;
  Handle console_in_handle;
  SimpleTextInputProtocol* con_in;
  Handle console_out_handle;
  SimpleTextOutputProtocol* con_out;
  Handle standard_error_handle;
  SimpleTextOutputProtocol* std_err;
  RuntimeServices* runtime_services;
  BootServices* boot_services;
  UINTN number_of_table_entries;
  ConfigurationTable* configuration_table;
};

enum _GRAPHICS_PIXEL_FORMAT {
  kPixelRedGreenBlueReserved8BitPerColor,
  kPixelBlueGreenRedReserved8BitPerColor,
  kPixelBitMask,
  kPixelBltOnly,
  kPixelFormatMax
};

struct GraphicsOutputProtocol {
  uint64_t _buf[3];
  struct {
    uint32_t max_mode;
    uint32_t mode;
    struct {
      uint32_t version;
      uint32_t horizontal_resolution;
      uint32_t vertical_resolution;
      uint32_t pixel_format;
      struct {
        uint32_t red_mask;
        uint32_t green_mask;
        uint32_t blue_mask;
        uint32_t reserved_mask;
      } pixel_info;
      uint32_t pixels_per_scan_line;
    } * info;
    UINTN size_of_info;
    void* frame_buffer_base;
    UINTN frame_buffer_size;
  } * mode;
};

struct _SIMPLE_POINTER_STATE {
  int RelativeMovementX;
  int RelativeMovementY;
  int RelativeMovementZ;
  unsigned char LeftButton;
  unsigned char RightButton;
};

struct _SIMPLE_POINTER_PROTOCOL {
  uint64_t (*Reset)(struct _SIMPLE_POINTER_PROTOCOL* This,
                    unsigned char ExtendedVerification);
  uint64_t (*GetState)(struct _SIMPLE_POINTER_PROTOCOL* This,
                       struct _SIMPLE_POINTER_STATE* State);
  void* WaitForInput;
};

struct KeyState {
  unsigned int KeyShiftState;
  unsigned char KeyToggleState;
};

struct _KEY_DATA {
  struct InputKey Key;
  struct KeyState KeyState;
};

struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL {
  uint64_t (*Reset)(struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                    unsigned char ExtendedVerification);
  uint64_t (*ReadKeyStrokeEx)(struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                              struct _KEY_DATA* KeyData);
  void* WaitForKeyEx;
  uint64_t (*SetState)(struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                       unsigned char* KeyToggleState);
  uint64_t (*RegisterKeyNotify)(
      struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
      struct _KEY_DATA* KeyData,
      uint64_t (*KeyNotificationFunction)(struct _KEY_DATA* KeyData),
      void** NotifyHandle);
  uint64_t (*UnregisterKeyNotify)(struct _SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                                  void* NotificationHandle);
};

struct _LOADED_IMAGE_PROTOCOL {
  unsigned int Revision;
  void* ParentHandle;
  struct SystemTable* SystemTable;
  // Source location of the image
  void* DeviceHandle;
  struct _DEVICE_PATH_PROTOCOL* FilePath;
  void* Reserved;
  // Image’s load options
  unsigned int LoadOptionsSize;
  void* LoadOptions;
  // Location where image was loaded
  void* ImageBase;
  uint64_t ImageSize;
  MemoryType ImageCodeType;
  MemoryType ImageDataType;
  uint64_t (*Unload)(void* ImageHandle);
};

struct _DEVICE_PATH_TO_TEXT_PROTOCOL {
  uint64_t _buf;
  unsigned short* (*ConvertDevicePathToText)(
      const struct _DEVICE_PATH_PROTOCOL* DeviceNode,
      unsigned char DisplayOnly,
      unsigned char AllowShortcuts);
};

struct _DEVICE_PATH_FROM_TEXT_PROTOCOL {
  struct _DEVICE_PATH_PROTOCOL* (*ConvertTextToDeviceNode)(
      const unsigned short* TextDeviceNode);
  struct _DEVICE_PATH_PROTOCOL* (*ConvertTextToDevicePath)(
      const unsigned short* TextDevicePath);
};

struct _DEVICE_PATH_UTILITIES_PROTOCOL {
  uint64_t _buf[3];
  struct _DEVICE_PATH_PROTOCOL* (*AppendDeviceNode)(
      const struct _DEVICE_PATH_PROTOCOL* DevicePath,
      const struct _DEVICE_PATH_PROTOCOL* DeviceNode);
};

class MemoryMap {
 public:
  void Init(void);
  void Print(void);
  int GetNumberOfEntries(void) const {
    return static_cast<int>(bytes_used_ / descriptor_size_);
  }
  const MemoryDescriptor* GetDescriptor(int index) const {
    return reinterpret_cast<const MemoryDescriptor*>(
        &buf_[index * descriptor_size_]);
  }
  UINTN GetKey(void) const { return key_; }
  static constexpr int kBufferSize = 4096;

 private:
  UINTN key_;
  UINTN bytes_used_;
  UINTN descriptor_size_;
  uint8_t buf_[kBufferSize];
};

extern SystemTable* system_table;
extern GraphicsOutputProtocol* graphics_output_protocol;
extern SimpleFileSystemProtocol* simple_fs;

namespace ConOut {
void ClearScreen();
void PutChar(wchar_t c);
};  // namespace ConOut

wchar_t GetChar();
void* GetConfigurationTableByUUID(const GUID* guid);
void GetMemoryMapAndExitBootServices(Handle image_handle, MemoryMap& map);
EFI::FileProtocol* OpenFile(const wchar_t* path);
void Init(SystemTable* system_table);
}  // namespace EFI
