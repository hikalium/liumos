#include <efi.h>
#include <common.h>

struct EFI_SYSTEM_TABLE *ST;
struct EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
struct EFI_SIMPLE_POINTER_PROTOCOL *SPP;
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;
struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *STIEP;
struct EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DPTTP;
struct EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL *DPFTP;
struct EFI_DEVICE_PATH_UTILITIES_PROTOCOL *DPUP;
struct EFI_GUID lip_guid = {0x5b1b31a1, 0x9562, 0x11d2,
			    {0x8e, 0x3f, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID dpp_guid = {0x09576e91, 0x6d3f, 0x11d2,
			    {0x8e, 0x39, 0x00, 0xa0,
			     0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID fi_guid = {0x09576e92, 0x6d3f, 0x11d2,
			   {0x8e, 0x39, 0x00, 0xa0,
			    0xc9, 0x69, 0x72, 0x3b}};

void efi_init(struct EFI_SYSTEM_TABLE *SystemTable)
{
	struct EFI_GUID gop_guid = {0x9042a9de, 0x23dc, 0x4a38,
				    {0x96, 0xfb, 0x7a, 0xde,
				     0xd0, 0x80, 0x51, 0x6a}};
	struct EFI_GUID spp_guid = {0x31878c87, 0xb75, 0x11d5,
				    {0x9a, 0x4f, 0x0, 0x90,
				     0x27, 0x3f, 0xc1, 0x4d}};
	struct EFI_GUID sfsp_guid = {0x0964e5b22, 0x6459, 0x11d2,
				     {0x8e, 0x39, 0x00, 0xa0,
				      0xc9, 0x69, 0x72, 0x3b}};
	struct EFI_GUID stiep_guid = {0xdd9e7534, 0x7762, 0x4698,
				      {0x8c, 0x14, 0xf5, 0x85,
				       0x17, 0xa6, 0x25, 0xaa}};
	struct EFI_GUID dpttp_guid = {0x8b843e20, 0x8132, 0x4852,
				      {0x90, 0xcc, 0x55, 0x1a,
				       0x4e, 0x4a, 0x7f, 0x1c}};
	struct EFI_GUID dpftp_guid = {0x5c99a21, 0xc70f, 0x4ad2,
				      {0x8a, 0x5f, 0x35, 0xdf,
				       0x33, 0x43, 0xf5, 0x1e}};
	struct EFI_GUID dpup_guid = {0x379be4e, 0xd706, 0x437d,
				     {0xb0, 0x37, 0xed, 0xb8,
				      0x2f, 0xb7, 0x72, 0xa4}};

	ST = SystemTable;
	ST->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	ST->BootServices->LocateProtocol(&gop_guid, NULL, (void **)&GOP);
	ST->BootServices->LocateProtocol(&spp_guid, NULL, (void **)&SPP);
	ST->BootServices->LocateProtocol(&sfsp_guid, NULL, (void **)&SFSP);
	ST->BootServices->LocateProtocol(&stiep_guid, NULL, (void **)&STIEP);
	ST->BootServices->LocateProtocol(&dpttp_guid, NULL, (void **)&DPTTP);
	ST->BootServices->LocateProtocol(&dpftp_guid, NULL, (void **)&DPFTP);
	ST->BootServices->LocateProtocol(&dpup_guid, NULL, (void **)&DPUP);
}
