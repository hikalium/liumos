#include <efi.h>
#include <common.h>
#include <mem.h>

#define MEM_DESC_SIZE	4800

unsigned char mem_desc[MEM_DESC_SIZE];
unsigned long long mem_desc_num;
unsigned long long mem_desc_unit_size;
unsigned long long map_key;

void dump_memmap(void)
{
	struct EFI_MEMORY_DESCRIPTOR *p =
		(struct EFI_MEMORY_DESCRIPTOR *)mem_desc;
	unsigned int i;

	for (i = 0; i < mem_desc_num; i++) {
		puth((unsigned long long)p, 16);
		putc(L' ');
		puth(p->Type, 2);
		putc(L' ');
		puth(p->PhysicalStart, 16);
		putc(L' ');
		puth(p->VirtualStart, 16);
		putc(L' ');
		puth(p->NumberOfPages, 16);
		putc(L' ');
		puth(p->Attribute, 16);
		puts(L"\r\n");

		p = (struct EFI_MEMORY_DESCRIPTOR *)(
			(unsigned char *)p + mem_desc_unit_size);
	}
}

void init_memmap(void)
{
	unsigned long long status;
	unsigned long long mmap_size = MEM_DESC_SIZE;
	unsigned int desc_ver;

	status = ST->BootServices->GetMemoryMap(
		&mmap_size, (struct EFI_MEMORY_DESCRIPTOR *)mem_desc, &map_key,
		&mem_desc_unit_size, &desc_ver);
	assert(status, L"GetMemoryMap");
	mem_desc_num = mmap_size / mem_desc_unit_size;
}

struct EFI_MEMORY_DESCRIPTOR *get_allocatable_area(unsigned long long size)
{
	struct EFI_MEMORY_DESCRIPTOR *p =
		(struct EFI_MEMORY_DESCRIPTOR *)mem_desc;
	unsigned long long i;

	for (i = 0; i < mem_desc_num; i++) {
		if ((p->Type == EfiConventionalMemory) &&
		    ((p->NumberOfPages * PAGE_SIZE) >= size))
			break;

		p = (struct EFI_MEMORY_DESCRIPTOR *)(
			(unsigned char *)p + mem_desc_unit_size);
	}

	return p;
}

void exit_boot_services(void *IH)
{
	unsigned long long status;
	unsigned long long mmap_size;
	unsigned int desc_ver;

	do {
		mmap_size = MEM_DESC_SIZE;
		status = ST->BootServices->GetMemoryMap(
			&mmap_size, (struct EFI_MEMORY_DESCRIPTOR *)mem_desc,
			&map_key, &mem_desc_unit_size, &desc_ver);
	} while (!check_warn_error(status, L"GetMemoryMap"));

	status = ST->BootServices->ExitBootServices(IH, map_key);
	assert(status, L"ExitBootServices");
}
