#pragma once

#include "acpi.h"
#include "asm.h"
#include "efi.h"
#include "generic.h"
#include "guid.h"

#define HPET_TIMER_CONFIG_REGISTER_BASE_OFS 0x100
#define HPET_TICK_PER_SEC (10UL * 1000 * 1000)

#define HPET_INT_TYPE_LEVEL_TRIGGERED (1UL << 1)
#define HPET_INT_ENABLE (1UL << 2)
#define HPET_MODE_PERIODIC (1UL << 3)
#define HPET_SET_VALUE (1UL << 6)

packed_struct HPETTimerRegisterSet {
  uint64_t configuration_and_capability;
  uint64_t comparator_value;
  uint64_t fsb_interrupt_route;
  uint64_t reserved;
};

packed_struct HPETRegisterSpace {
  uint64_t general_capabilities_and_id;
  uint64_t reserved00;
  uint64_t general_configuration;
  uint64_t reserved01;
  uint64_t general_interrupt_status;
  uint64_t reserved02;
  uint64_t reserved03[24];
  uint64_t main_counter_value;
  uint64_t reserved04;
  HPETTimerRegisterSet timers[32];
};

// @console.c
void ResetCursorPosition();
void EnableVideoModeForConsole();
void PutChar(char c);
void PutString(const char* s);
void PutChars(const char* s, int n);
void PutHex64(uint64_t value);
void PutStringAndHex(const char* s, uint64_t value);

// @draw.c
void DrawCharacter(char c, int px, int py);
void DrawRect(int px, int py, int w, int h, uint32_t col);
void BlockTransfer(int to_x, int to_y, int from_x, int from_y, int w, int h);

// @font.gen.c
extern uint8_t font[0x100][16];

// @static.c
extern const GUID EFI_ACPITableGUID;
extern const GUID EFI_GraphicsOutputProtocolGUID;
