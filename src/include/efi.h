#ifndef _EFI_H_
#define _EFI_H_

#define EFI_FILE_MODE_READ	0x0000000000000001
#define EFI_FILE_MODE_WRITE	0x0000000000000002
#define EFI_FILE_MODE_CREATE	0x8000000000000000

#define EFI_FILE_READ_ONLY	0x0000000000000001

//*******************************************************
// Attributes
//*******************************************************
#define EFI_BLACK		0x00
#define EFI_BLUE		0x01
#define EFI_GREEN		0x02
#define EFI_CYAN		0x03
#define EFI_RED			0x04
#define EFI_MAGENTA		0x05
#define EFI_BROWN		0x06
#define EFI_LIGHTGRAY		0x07
#define EFI_BRIGHT		0x08
#define EFI_DARKGRAY		0x08
#define EFI_LIGHTBLUE		0x09
#define EFI_LIGHTGREEN		0x0A
#define EFI_LIGHTCYAN		0x0B
#define EFI_LIGHTRED		0x0C
#define EFI_LIGHTMAGENTA	0x0D
#define EFI_YELLOW		0x0E
#define EFI_WHITE		0x0F

#define EFI_BACKGROUND_BLACK		0x00
#define EFI_BACKGROUND_BLUE		0x10
#define EFI_BACKGROUND_GREEN		0x20
#define EFI_BACKGROUND_CYAN		0x30
#define EFI_BACKGROUND_RED		0x40
#define EFI_BACKGROUND_MAGENTA		0x50
#define EFI_BACKGROUND_BROWN		0x60
#define EFI_BACKGROUND_LIGHTGRAY	0x70

#define EFI_SUCCESS	0
#define EFI_ERROR	0x8000000000000000
#define EFI_UNSUPPORTED	(EFI_ERROR | 3)

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER		0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE		0x00000020

#define EVT_TIMER				0x80000000
#define EVT_RUNTIME				0x40000000
#define EVT_NOTIFY_WAIT				0x00000100
#define EVT_NOTIFY_SIGNAL			0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

#define TPL_APPLICATION	4
#define TPL_CALLBACK	8
#define TPL_NOTIFY	16
#define TPL_HIGH_LEVEL	31

struct EFI_INPUT_KEY {
	unsigned short ScanCode;
	unsigned short UnicodeChar;
};

struct EFI_GUID {
	unsigned int Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
};

enum EFI_MEMORY_TYPE {
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
};

enum EFI_TIMER_DELAY {
	TimerCancel,
	TimerPeriodic,
	TimerRelative
};

enum EFI_RESET_TYPE {
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown,
	EfiResetPlatformSpecific
};

struct EFI_DEVICE_PATH_PROTOCOL {
	unsigned char Type;
	unsigned char SubType;
	unsigned char Length[2];
};

struct EFI_MEMORY_DESCRIPTOR {
	unsigned int Type;
	unsigned long long PhysicalStart;
	unsigned long long VirtualStart;
	unsigned long long NumberOfPages;
	unsigned long long Attribute;
};

struct EFI_SYSTEM_TABLE {
	char _buf1[44];
	struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
		unsigned long long _buf;
		unsigned long long (*ReadKeyStroke)(
			struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
			struct EFI_INPUT_KEY *Key);
		void *WaitForKey;
	} *ConIn;
	unsigned long long _buf2;
	struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
		unsigned long long _buf;
		unsigned long long (*OutputString)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned short *String);
		unsigned long long (*TestString)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned short *String);
		unsigned long long (*QueryMode)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned long long ModeNumber,
			unsigned long long *Columns,
			unsigned long long *Rows);
		unsigned long long (*SetMode)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned long long ModeNumber);
		unsigned long long (*SetAttribute)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
			unsigned long long Attribute);
		unsigned long long (*ClearScreen)(
			struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
		unsigned long long _buf4[2];
		struct SIMPLE_TEXT_OUTPUT_MODE {
			int MaxMode;
			int Mode;
			int Attribute;
			int CursorColumn;
			int CursorRow;
			unsigned char CursorVisible;
		} *Mode;
	} *ConOut;
	unsigned long long _buf3[2];
	struct EFI_RUNTIME_SERVICES {
		char _buf_rs1[24];

		//
		// Time Services
		//
		unsigned long long _buf_rs2[4];

		//
		// Virtual Memory Services
		//
		unsigned long long _buf_rs3[2];

		//
		// Variable Services
		//
		unsigned long long _buf_rs4[3];

		//
		// Miscellaneous Services
		//
		unsigned long long _buf_rs5;
		void (*ResetSystem)(enum EFI_RESET_TYPE ResetType,
				    unsigned long long ResetStatus,
				    unsigned long long DataSize,
				    void *ResetData);
	} *RuntimeServices;
	struct EFI_BOOT_SERVICES {
		char _buf1[24];

		//
		// Task Priority Services
		//
		unsigned long long _buf2[2];

		//
		// Memory Services
		//
		unsigned long long _buf3[2];
		unsigned long long (*GetMemoryMap)(
			unsigned long long *MemoryMapSize,
			struct EFI_MEMORY_DESCRIPTOR *MemoryMap,
			unsigned long long *MapKey,
			unsigned long long *DescriptorSize,
			unsigned int *DescriptorVersion);
		unsigned long long (*AllocatePool)(
			enum EFI_MEMORY_TYPE PoolType,
			unsigned long long Size,
			void **Buffer);
		unsigned long long (*FreePool)(
			void *Buffer);

		//
		// Event & Timer Services
		//
		unsigned long long (*CreateEvent)(
			unsigned int Type,
			unsigned long long NotifyTpl,
			void (*NotifyFunction)(void *Event, void *Context),
			void *NotifyContext,
			void *Event);
		unsigned long long (*SetTimer)(void *Event,
					       enum EFI_TIMER_DELAY Type,
					       unsigned long long TriggerTime);
		unsigned long long (*WaitForEvent)(
			unsigned long long NumberOfEvents,
			void **Event,
			unsigned long long *Index);
		unsigned long long _buf4_2[3];

		//
		// Protocol Handler Services
		//
		unsigned long long _buf5[9];

		//
		// Image Services
		//
		unsigned long long (*LoadImage)(
			unsigned char BootPolicy,
			void *ParentImageHandle,
			struct EFI_DEVICE_PATH_PROTOCOL *DevicePath,
			void *SourceBuffer,
			unsigned long long SourceSize,
			void **ImageHandle);
		unsigned long long (*StartImage)(
			void *ImageHandle,
			unsigned long long *ExitDataSize,
			unsigned short **ExitData);
		unsigned long long _buf6[2];
		unsigned long long (*ExitBootServices)(
			void *ImageHandle,
			unsigned long long MapKey);

		//
		// Miscellaneous Services
		//
		unsigned long long _buf7[2];
		unsigned long long (*SetWatchdogTimer)(
			unsigned long long Timeout,
			unsigned long long WatchdogCode,
			unsigned long long DataSize,
			unsigned short *WatchdogData);

		//
		// DriverSupport Services
		//
		unsigned long long _buf8[2];

		//
		// Open and Close Protocol Services
		//
		unsigned long long (*OpenProtocol)(
			void *Handle,
			struct EFI_GUID *Protocol,
			void **Interface,
			void *AgentHandle,
			void *ControllerHandle,
			unsigned int Attributes);
		unsigned long long _buf9[2];

		//
		// Library Services
		//
		unsigned long long _buf10[2];
		unsigned long long (*LocateProtocol)(
			struct EFI_GUID *Protocol,
			void *Registration,
			void **Interface);
		unsigned long long _buf10_2[2];

		//
		// 32-bit CRC Services
		//
		unsigned long long _buf11;

		//
		// Miscellaneous Services
		//
		void (*CopyMem)(
			void *Destination,
			void *Source,
			unsigned long long Length);
		void (*SetMem)(
			void *Buffer,
			unsigned long long Size,
			unsigned char Value);
		unsigned long long _buf12;
	} *BootServices;
};

struct EFI_GRAPHICS_OUTPUT_BLT_PIXEL {
	unsigned char Blue;
	unsigned char Green;
	unsigned char Red;
	unsigned char Reserved;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
	unsigned long long _buf[3];
	struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
		unsigned int MaxMode;
		unsigned int Mode;
		struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
			unsigned int Version;
			unsigned int HorizontalResolution;
			unsigned int VerticalResolution;
			enum EFI_GRAPHICS_PIXEL_FORMAT {
				PixelRedGreenBlueReserved8BitPerColor,
				PixelBlueGreenRedReserved8BitPerColor,
				PixelBitMask,
				PixelBltOnly,
				PixelFormatMax
			} PixelFormat;
		} *Info;
		unsigned long long SizeOfInfo;
		unsigned long long FrameBufferBase;
		unsigned long long FrameBufferSize;
	} *Mode;
};

struct EFI_SIMPLE_POINTER_STATE {
	int RelativeMovementX;
	int RelativeMovementY;
	int RelativeMovementZ;
	unsigned char LeftButton;
	unsigned char RightButton;
};

struct EFI_SIMPLE_POINTER_PROTOCOL {
	unsigned long long (*Reset)(
		struct EFI_SIMPLE_POINTER_PROTOCOL *This,
		unsigned char ExtendedVerification);
	unsigned long long (*GetState)(
		struct EFI_SIMPLE_POINTER_PROTOCOL *This,
		struct EFI_SIMPLE_POINTER_STATE *State);
	void *WaitForInput;
};

struct EFI_TIME {
	unsigned short Year; // 1900 – 9999
	unsigned char Month; // 1 – 12
	unsigned char Day; // 1 – 31
	unsigned char Hour; // 0 – 23
	unsigned char Minute; // 0 – 59
	unsigned char Second; // 0 – 59
	unsigned char Pad1;
	unsigned int Nanosecond; // 0 – 999,999,999
	unsigned short TimeZone; // -1440 to 1440 or 2047
	unsigned char Daylight;
	unsigned char Pad2;
};

struct EFI_FILE_INFO {
	unsigned long long Size;
	unsigned long long FileSize;
	unsigned long long PhysicalSize;
	struct EFI_TIME CreateTime;
	struct EFI_TIME LastAccessTime;
	struct EFI_TIME ModificationTime;
	unsigned long long Attribute;
	unsigned short FileName[];
};

struct EFI_FILE_PROTOCOL {
	unsigned long long _buf;
	unsigned long long (*Open)(struct EFI_FILE_PROTOCOL *This,
				   struct EFI_FILE_PROTOCOL **NewHandle,
				   unsigned short *FileName,
				   unsigned long long OpenMode,
				   unsigned long long Attributes);
	unsigned long long (*Close)(struct EFI_FILE_PROTOCOL *This);
	unsigned long long _buf2;
	unsigned long long (*Read)(struct EFI_FILE_PROTOCOL *This,
				   unsigned long long *BufferSize,
				   void *Buffer);
	unsigned long long (*Write)(struct EFI_FILE_PROTOCOL *This,
				    unsigned long long *BufferSize,
				    void *Buffer);
	unsigned long long _buf3[2];
	unsigned long long (*GetInfo)(struct EFI_FILE_PROTOCOL *This,
				      struct EFI_GUID *InformationType,
				      unsigned long long *BufferSize,
				      void *Buffer);
	unsigned long long _buf4;
	unsigned long long (*Flush)(struct EFI_FILE_PROTOCOL *This);
};

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
	unsigned long long Revision;
	unsigned long long (*OpenVolume)(
		struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
		struct EFI_FILE_PROTOCOL **Root);
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
	unsigned long long (*Reset)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		unsigned char ExtendedVerification);
	unsigned long long (*ReadKeyStrokeEx)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		struct EFI_KEY_DATA *KeyData);
	void *WaitForKeyEx;
	unsigned long long (*SetState)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		unsigned char *KeyToggleState);
	unsigned long long (*RegisterKeyNotify)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		struct EFI_KEY_DATA *KeyData,
		unsigned long long (*KeyNotificationFunction)(
			struct EFI_KEY_DATA *KeyData),
		void **NotifyHandle);
	unsigned long long (*UnregisterKeyNotify)(
		struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
		void *NotificationHandle);
};

struct EFI_LOADED_IMAGE_PROTOCOL {
	unsigned int Revision;
	void *ParentHandle;
	struct EFI_SYSTEM_TABLE *SystemTable;
	// Source location of the image
	void *DeviceHandle;
	struct EFI_DEVICE_PATH_PROTOCOL *FilePath;
	void *Reserved;
	// Image’s load options
	unsigned int LoadOptionsSize;
	void *LoadOptions;
	// Location where image was loaded
	void *ImageBase;
	unsigned long long ImageSize;
	enum EFI_MEMORY_TYPE ImageCodeType;
	enum EFI_MEMORY_TYPE ImageDataType;
	unsigned long long (*Unload)(void *ImageHandle);
};

struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL {
	unsigned long long _buf;
	unsigned short *(*ConvertDevicePathToText)(
		const struct EFI_DEVICE_PATH_PROTOCOL* DeviceNode,
		unsigned char DisplayOnly,
		unsigned char AllowShortcuts);
};

struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL {
	struct EFI_DEVICE_PATH_PROTOCOL *(*ConvertTextToDeviceNode) (
		const unsigned short *TextDeviceNode);
	struct EFI_DEVICE_PATH_PROTOCOL *(*ConvertTextToDevicePath) (
		const unsigned short *TextDevicePath);
};

struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL {
	unsigned long long _buf[3];
	struct EFI_DEVICE_PATH_PROTOCOL *(*AppendDeviceNode)(
		const struct EFI_DEVICE_PATH_PROTOCOL *DevicePath,
		const struct EFI_DEVICE_PATH_PROTOCOL *DeviceNode);
};

extern struct EFI_SYSTEM_TABLE *ST;
extern struct EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
extern struct EFI_SIMPLE_POINTER_PROTOCOL *SPP;
extern struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;
extern struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *STIEP;
extern struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DPTTP;
extern struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *DPFTP;
extern struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL *DPUP;
extern struct EFI_GUID lip_guid;
extern struct EFI_GUID dpp_guid;
extern struct EFI_GUID fi_guid;

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable);

#endif
