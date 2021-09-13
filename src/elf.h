#pragma once

#include <elf.h>

#include "generic.h"
#include "loader_info.h"

class EFIFile;
class Process;
class PersistentMemoryManager;

const Elf64_Shdr* FindSectionHeader(EFIFile& file, const char* name);

Process& LoadELFAndCreateEphemeralProcess(EFIFile& file, const char* const name);
Process& LoadELFAndCreatePersistentProcess(EFIFile& file,
                                           PersistentMemoryManager& pmem);
void LoadKernelELF(EFIFile& liumos_elf, LoaderInfo&);
