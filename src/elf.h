#pragma once

#include "generic.h"

class File;
class Process;
class PersistentMemoryManager;

Process& LoadELFAndLaunchEphemeralProcess(File& file);
Process& LoadELFAndLaunchPersistentProcess(File& file,
                                           PersistentMemoryManager& pmem);
void LoadKernelELF(File& file);
