#pragma once

#include "generic.h"
#include "guid.h"

#define EFI_MEMORY_NV 0x8000

typedef enum {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown,
  EfiResetPlatformSpecific
} EFIResetType;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiMaxMemoryType
} EFIMemoryType;

typedef enum { TimerCancel, TimerPeriodic, TimerRelative } EFITimerDelay;

typedef uint64_t EFI_UINTN;
typedef void* EFIHandle;

typedef struct EFI_CONFIGURATION_TABLE EFIConfigurationTable;
typedef struct EFI_TABLE_HEADER EFITableHeader;
typedef struct EFI_INPUT_KEY EFIInputKey;
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFISimpleTextInputProtocol;
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFISimpleTextOutputProtocol;
typedef struct EFI_RUNTIME_SERVICES EFIRuntimeServices;
typedef struct EFI_MEMORY_DESCRIPTOR EFIMemoryDescriptor;
typedef struct EFI_DEVICE_PATH_PROTOCOL EFIDevicePathProtocol;
typedef struct EFI_BOOT_SERVICES EFIBootServices;
typedef struct EFI_SYSTEM_TABLE EFISystemTable;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFIGraphicsOutputProtocol;

struct EFI_CONFIGURATION_TABLE {
  GUID vendor_guid;
  void* vendor_table;
};

struct EFI_TABLE_HEADER {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc32;
  uint32_t reserved;
};

struct EFI_INPUT_KEY {
  uint16_t ScanCode;
  wchar_t UnicodeChar;
};

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  uint64_t _buf;
  uint64_t (*ReadKeyStroke)(EFISimpleTextInputProtocol*, EFIInputKey*);
  void* wait_for_key;
};

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  uint64_t _buf;
  uint64_t (*output_string)(EFISimpleTextOutputProtocol*, wchar_t*);
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

struct EFI_RUNTIME_SERVICES {
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

struct EFI_MEMORY_DESCRIPTOR {
  unsigned int type;
  uint64_t physical_start;
  uint64_t virtual_start;
  uint64_t number_of_pages;
  uint64_t attribute;
};

struct EFI_DEVICE_PATH_PROTOCOL {
  unsigned char Type;
  unsigned char SubType;
  unsigned char Length[2];
};

struct EFI_BOOT_SERVICES {
  char _buf1[24];
  uint64_t _buf2[2];
  uint64_t _buf3[2];
  uint64_t (*GetMemoryMap)(uint64_t* memory_map_size,
                           EFIMemoryDescriptor*,
                           uint64_t* map_key,
                           uint64_t* descriptor_size,
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
  uint64_t (*ExitBootServices)(void* ImageHandle, uint64_t MapKey);
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

struct EFI_SYSTEM_TABLE {
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

struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL {
  unsigned char Blue;
  unsigned char Green;
  unsigned char Red;
  unsigned char Reserved;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
  uint64_t _buf[3];
  struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
    unsigned int max_mode;
    unsigned int mode;
    struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
      unsigned int version;
      unsigned int horizontal_resolution;
      unsigned int vertical_resolution;
      enum EFI_GRAPHICS_PIXEL_FORMAT {
        kPixelRedGreenBlueReserved8BitPerColor,
        kPixelBlueGreenRedReserved8BitPerColor,
        kPixelBitMask,
        kPixelBltOnly,
        kPixelFormatMax
      } pixel_format;
    } * info;
    uint64_t size_of_info;
    void* frame_buffer_base;
    uint64_t frame_buffer_size;
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

struct EFI_TIME {
  unsigned short Year;   // 1900 – 9999
  unsigned char Month;   // 1 – 12
  unsigned char Day;     // 1 – 31
  unsigned char Hour;    // 0 – 23
  unsigned char Minute;  // 0 – 59
  unsigned char Second;  // 0 – 59
  unsigned char Pad1;
  unsigned int Nanosecond;  // 0 – 999,999,999
  unsigned short TimeZone;  // -1440 to 1440 or 2047
  unsigned char Daylight;
  unsigned char Pad2;
};

struct EFI_FILE_INFO {
  uint64_t Size;
  uint64_t FileSize;
  uint64_t PhysicalSize;
  struct EFI_TIME CreateTime;
  struct EFI_TIME LastAccessTime;
  struct EFI_TIME ModificationTime;
  uint64_t Attribute;
  unsigned short FileName[];
};

struct EFI_FILE_PROTOCOL {
  uint64_t _buf;
  uint64_t (*Open)(struct EFI_FILE_PROTOCOL* This,
                   struct EFI_FILE_PROTOCOL** NewHandle,
                   unsigned short* FileName,
                   uint64_t OpenMode,
                   uint64_t Attributes);
  uint64_t (*Close)(struct EFI_FILE_PROTOCOL* This);
  uint64_t _buf2;
  uint64_t (*Read)(struct EFI_FILE_PROTOCOL* This,
                   uint64_t* BufferSize,
                   void* Buffer);
  uint64_t (*Write)(struct EFI_FILE_PROTOCOL* This,
                    uint64_t* BufferSize,
                    void* Buffer);
  uint64_t _buf3[2];
  uint64_t (*GetInfo)(struct EFI_FILE_PROTOCOL* This,
                      GUID* InformationType,
                      uint64_t* BufferSize,
                      void* Buffer);
  uint64_t _buf4;
  uint64_t (*Flush)(struct EFI_FILE_PROTOCOL* This);
};

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  uint64_t Revision;
  uint64_t (*OpenVolume)(struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* This,
                         struct EFI_FILE_PROTOCOL** Root);
};

struct EFI_KEY_STATE {
  unsigned int KeyShiftState;
  unsigned char KeyToggleState;
};

struct EFI_KEY_DATA {
  struct EFI_INPUT_KEY Key;
  struct EFI_KEY_STATE KeyState;
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

bool IsEqualStringWithSize(const char* s1, const char* s2, int n);
void EFIClearScreen();
void EFIPutChar(wchar_t c);
void EFIPutString(wchar_t* s);
void EFIPutCString(char* s);
void EFIPutnCString(char* s, int n);
wchar_t EFIGetChar();
void EFIGetMemoryMap();
void EFIPrintHex64(uint64_t value);
void EFIPrintStringAndHex(wchar_t* s, uint64_t value);
void EFIPrintMemoryDescriptor(EFIMemoryDescriptor* desc);
void EFIPrintMemoryMap();
void* EFIGetConfigurationTableByUUID(const GUID* guid);
