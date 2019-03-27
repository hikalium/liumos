#pragma once

#include "generic.h"

class File;
class Process;
class PersistentMemoryManager;

Process& LoadELFAndCreateEphemeralProcess(File& file);
Process& LoadELFAndCreatePersistentProcess(File& file,
                                           PersistentMemoryManager& pmem);
void LoadKernelELF(File& file);
