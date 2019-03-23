#pragma once

#include "lib/musl/include/elf.h"

#include "execution_context.h"
#include "generic.h"

class File;
class Process;

Process& LoadELFAndLaunchEphemeralProcess(File& file);
Process& LoadELFAndLaunchPersistentProcess(File& file);
void LoadKernelELF(File& file);
