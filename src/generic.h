#pragma once

#include <stddef.h>
#include <stdint.h>

#define packed_struct struct __attribute__((__packed__))
#define packed_union union __attribute__((__packed__))
