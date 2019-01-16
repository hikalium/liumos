#pragma once

#include "generic.h"
#include "guid.h"

#define EFI_MEMORY_NV 0x8000

typedef uint64_t EFI_UINTN;
typedef void* EFIHandle;

enum class EFIStatus : EFI_UINTN {
  kSuccess,
};

typedef enum {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown,
  EfiResetPlatformSpecific
} EFIResetType;

enum class EFIMemoryType : uint32_t {
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

typedef enum { TimerCancel, TimerPeriodic, TimerRelative } EFITimerDelay;

struct EFIConfigurationTable {
  GUID vendor_guid;
  void* vendor_table;
};

struct EFITableHeader {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc32;
  uint32_t reserved;
};

struct EFIInputKey {
  uint16_t ScanCode;
  wchar_t UnicodeChar;
};

struct EFISimpleTextInputProtocol {
  uint64_t _buf;
  uint64_t (*ReadKeyStroke)(EFISimpleTextInputProtocol*, EFIInputKey*);
  void* wait_for_key;
};

struct EFISimpleTextOutputProtocol {
  uint64_t _buf;
  uint64_t (*output_string)(EFISimpleTextOutputProtocol*, const wchar_t*);
  uint64_t (*test_string)(EFISimpleTextOutputProtocol*, wchar_t*);
  uint64_t (*query_mode)(EFISimpleTextOutputProtocol*,
                         wchar_t*,
                         uint64_t* columns,
                         uint64_t* rows);
  uint64_t (*set_mode)(EFISimpleTextOutputProtocol*, uint64_t);
  uint64_t (*set_attribute)(EFISimpleTextOutputProtocol*, uint64_t Attribute);
  uint64_t (*clear_screen)(EFISimpleTextOutputProtocol*);
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

struct EFITime {
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

struct EFIFileInfo {
  uint64_t Size;
  uint64_t FileSize;
  uint64_t PhysicalSize;
  EFITime CreateTime;
  EFITime LastAccessTime;
  EFITime ModificationTime;
  uint64_t Attribute;
  wchar_t file_name[1];
};

enum EFIFileProtocolModes : uint64_t {
  kRead = 1,
};

struct EFIFileProtocol {
  uint64_t revision;
  EFIStatus (*Open)(EFIFileProtocol* self,
                    EFIFileProtocol** new_handle,
                    const wchar_t* rel_path,
                    EFIFileProtocolModes mode,
                    uint64_t attr);
  EFIStatus (*Close)();
  EFIStatus (*Delete)();
  EFIStatus (*Read)(EFIFileProtocol* self,
                    EFI_UINTN* buffer_size,
                    uint8_t* buffer);
  uint64_t (*Write)();
  uint64_t _buf3[2];
  uint64_t (*GetInfo)();
  uint64_t _buf4;
  uint64_t (*Flush)();
};

struct EFISimpleFileSystemProtocol {
  uint64_t Revision;
  EFIStatus (*OpenVolume)(EFISimpleFileSystemProtocol* self,
                          EFIFileProtocol** Root);
};

struct EFIRuntimeServices {
  char _buf_rs1[24];
  uint64_t _buf_rs2[4];
  uint64_t _buf_rs3[2];
  uint64_t _buf_rs4[3];
  uint64_t _buf_rs5;
  void (*reset_system)(EFIResetType,
                       uint64_t reset_status,
                       uint64_t data_size,
                       void*);
};

struct EFIMemoryDescriptor {
  EFIMemoryType type;
  uint64_t physical_start;
  uint64_t virtual_start;
  uint64_t number_of_pages;
  uint64_t attribute;

  void Print(void) const;
};

struct EFIDevicePathProtocol {
  unsigned char Type;
  unsigned char SubType;
  unsigned char Length[2];
};

struct EFIBootServices {
  char _buf1[24];
  uint64_t _buf2[2];
  uint64_t _buf3[2];
  EFIStatus (*GetMemoryMap)(EFI_UINTN* memory_map_size,
                            uint8_t*,
                            EFI_UINTN* map_key,
                            EFI_UINTN* descriptor_size,
                            uint32_t* descriptor_version);
  uint64_t (*AllocatePool)(EFIMemoryType, uint64_t, void**);
  uint64_t (*FreePool)(void* Buffer);
  uint64_t (*CreateEvent)(unsigned int Type,
                          uint64_t NotifyTpl,
                          void (*NotifyFunction)(void* Event, void* Context),
                          void* NotifyContext,
                          void* Event);
  uint64_t (*SetTimer)(void* Event, EFITimerDelay, uint64_t TriggerTime);
  uint64_t (*WaitForEvent)(uint64_t NumberOfEvents,
                           void** Event,
                           uint64_t* Index);
  uint64_t _buf4_2[3];
  uint64_t _buf5[9];
  uint64_t (*LoadImage)(unsigned char BootPolicy,
                        void* ParentImageHandle,
                        EFIDevicePathProtocol*,
                        void* SourceBuffer,
                        uint64_t SourceSize,
                        void** ImageHandle);
  uint64_t (*StartImage)(void* ImageHandle,
                         uint64_t* ExitDataSize,
                         unsigned short** ExitData);
  uint64_t _buf6[2];
  EFIStatus (*ExitBootServices)(void* image_handle, EFI_UINTN map_key);
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

struct EFISystemTable {
  EFITableHeader header;
  wchar_t* firmware_vendor;
  uint32_t firmware_revision;
  EFIHandle console_in_handle;
  EFISimpleTextInputProtocol* con_in;
  EFIHandle console_out_handle;
  EFISimpleTextOutputProtocol* con_out;
  EFIHandle standard_error_handle;
  EFISimpleTextOutputProtocol* std_err;
  EFIRuntimeServices* runtime_services;
  EFIBootServices* boot_services;
  EFI_UINTN number_of_table_entries;
  EFIConfigurationTable* configuration_table;
};

enum EFI_GRAPHICS_PIXEL_FORMAT {
  kPixelRedGreenBlueReserved8BitPerColor,
  kPixelBlueGreenRedReserved8BitPerColor,
  kPixelBitMask,
  kPixelBltOnly,
  kPixelFormatMax
};

struct EFIGraphicsOutputProtocol {
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
    EFI_UINTN size_of_info;
    void* frame_buffer_base;
    EFI_UINTN frame_buffer_size;
  } * mode;
};

struct EFI_SIMPLE_POINTER_STATE {
  int RelativeMovementX;
  int RelativeMovementY;
  int RelativeMovementZ;
  unsigned char LeftButton;
  unsigned char RightButton;
};

struct EFI_SIMPLE_POINTER_PROTOCOL {
  uint64_t (*Reset)(struct EFI_SIMPLE_POINTER_PROTOCOL* This,
                    unsigned char ExtendedVerification);
  uint64_t (*GetState)(struct EFI_SIMPLE_POINTER_PROTOCOL* This,
                       struct EFI_SIMPLE_POINTER_STATE* State);
  void* WaitForInput;
};

struct EFIKeyState {
  unsigned int KeyShiftState;
  unsigned char KeyToggleState;
};

struct EFI_KEY_DATA {
  struct EFIInputKey Key;
  struct EFIKeyState KeyState;
};

struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
  uint64_t (*Reset)(struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                    unsigned char ExtendedVerification);
  uint64_t (*ReadKeyStrokeEx)(struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                              struct EFI_KEY_DATA* KeyData);
  void* WaitForKeyEx;
  uint64_t (*SetState)(struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
                       unsigned char* KeyToggleState);
  uint64_t (*RegisterKeyNotify)(
      struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
      struct EFI_KEY_DATA* KeyData,
      uint64_t (*KeyNotificationFunction)(struct EFI_KEY_DATA* KeyData),
      void** NotifyHandle);
  uint64_t (*UnregisterKeyNotify)(
      struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* This,
      void* NotificationHandle);
};

struct EFI_LOADED_IMAGE_PROTOCOL {
  unsigned int Revision;
  void* ParentHandle;
  struct EFI_SYSTEM_TABLE* SystemTable;
  // Source location of the image
  void* DeviceHandle;
  struct EFI_DEVICE_PATH_PROTOCOL* FilePath;
  void* Reserved;
  // Image’s load options
  unsigned int LoadOptionsSize;
  void* LoadOptions;
  // Location where image was loaded
  void* ImageBase;
  uint64_t ImageSize;
  EFIMemoryType ImageCodeType;
  EFIMemoryType ImageDataType;
  uint64_t (*Unload)(void* ImageHandle);
};

struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL {
  uint64_t _buf;
  unsigned short* (*ConvertDevicePathToText)(
      const struct EFI_DEVICE_PATH_PROTOCOL* DeviceNode,
      unsigned char DisplayOnly,
      unsigned char AllowShortcuts);
};

struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL {
  struct EFI_DEVICE_PATH_PROTOCOL* (*ConvertTextToDeviceNode)(
      const unsigned short* TextDeviceNode);
  struct EFI_DEVICE_PATH_PROTOCOL* (*ConvertTextToDevicePath)(
      const unsigned short* TextDevicePath);
};

struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL {
  uint64_t _buf[3];
  struct EFI_DEVICE_PATH_PROTOCOL* (*AppendDeviceNode)(
      const struct EFI_DEVICE_PATH_PROTOCOL* DevicePath,
      const struct EFI_DEVICE_PATH_PROTOCOL* DeviceNode);
};

extern EFISystemTable* _system_table;
extern EFIGraphicsOutputProtocol* efi_graphics_output_protocol;
extern ACPI_RSDT* rsdt;
extern EFISimpleFileSystemProtocol* efi_simple_fs;

const int kMemoryMapBufferSize = 4096;
class EFIMemoryMap {
 public:
  void Init(void);
  void Print(void);
  int GetNumberOfEntries(void) const {
    return static_cast<int>(bytes_used_ / descriptor_size_);
  }
  const EFIMemoryDescriptor* GetDescriptor(int index) const {
    return reinterpret_cast<const EFIMemoryDescriptor*>(
        &buf_[index * descriptor_size_]);
  }
  EFI_UINTN GetKey(void) const { return key_; }

 private:
  EFI_UINTN key_;
  EFI_UINTN bytes_used_;
  EFI_UINTN descriptor_size_;
  uint8_t buf_[kMemoryMapBufferSize];
};

bool IsEqualStringWithSize(const char* s1, const char* s2, int n);
void EFIClearScreen();
void EFIPutChar(wchar_t c);
void EFIPutString(const wchar_t* s);
void EFIPutCString(const char* s);
void EFIPutnCString(const char* s, int n);
wchar_t EFIGetChar();
void EFIPrintHex64(uint64_t value);
void EFIPrintStringAndHex(const wchar_t* s, uint64_t value);
void* EFIGetConfigurationTableByUUID(const GUID* guid);
void EFIGetMemoryMapAndExitBootServices(EFIHandle image_handle,
                                        EFIMemoryMap& map);
void InitEFI(EFISystemTable* system_table);
