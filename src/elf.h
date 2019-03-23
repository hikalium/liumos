#pragma once

#include "generic.h"

class File;
class Process;

Process& LoadELFAndLaunchEphemeralProcess(File& file);
void LoadKernelELF(File& file);
