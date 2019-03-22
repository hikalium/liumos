#pragma once

#include "generic.h"

class File;
class Process;

Process& LoadELFAndLaunchProcess(File& file);
void LoadKernelELF(File& file);
