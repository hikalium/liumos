#pragma once

#include "generic.h"

#include "lib/musl/include/elf.h"

class EFIFile;
class Process;
class PersistentMemoryManager;

const Elf64_Shdr* FindSectionHeader(EFIFile& file, const char* name);

Process& LoadELFAndCreateEphemeralProcess(EFIFile& file);
Process& LoadELFAndCreatePersistentProcess(EFIFile& file,
                                           PersistentMemoryManager& pmem);
void LoadKernelELF(EFIFile& file);
