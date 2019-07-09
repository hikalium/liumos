#pragma once

#include "generic.h"

#include "lib/musl/include/elf.h"

class File;
class Process;
class PersistentMemoryManager;

const Elf64_Shdr* FindSectionHeader(File& file, const char* name);

Process& LoadELFAndCreateEphemeralProcess(File& file);
Process& LoadELFAndCreatePersistentProcess(File& file,
                                           PersistentMemoryManager& pmem);
void LoadKernelELF(File& file);
