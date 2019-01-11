#include "liumos.h"

#define KEYID_MASK_ID 0x007f
#define KEYID_MASK_BREAK KEYCODE_MASK_BREAK
#define KEYID_MASK_TENKEY 0x0100
#define KEYID_MASK_STATE_SHIFT 0x0200
#define KEYID_MASK_STATE_CTRL 0x0400
#define KEYID_MASK_STATE_ALT 0x0800
#define KEYID_MASK_STATE_LOCK_SCROOL 0x1000
#define KEYID_MASK_STATE_LOCK_NUM 0x2000
#define KEYID_MASK_STATE_LOCK_CAPS 0x4000

#define KEYID_ESC (kKeyIDMaskExtended | 0x0000)
#define KEYID_F1 (kKeyIDMaskExtended | 0x0001)
#define KEYID_F2 (kKeyIDMaskExtended | 0x0002)
#define KEYID_F3 (kKeyIDMaskExtended | 0x0003)
#define KEYID_F4 (kKeyIDMaskExtended | 0x0004)
#define KEYID_F5 (kKeyIDMaskExtended | 0x0005)
#define KEYID_F6 (kKeyIDMaskExtended | 0x0006)
#define KEYID_F7 (kKeyIDMaskExtended | 0x0007)
#define KEYID_F8 (kKeyIDMaskExtended | 0x0008)
#define KEYID_F9 (kKeyIDMaskExtended | 0x0009)
#define KEYID_F10 (kKeyIDMaskExtended | 0x000a)
#define KEYID_F11 (kKeyIDMaskExtended | 0x000b)
#define KEYID_F12 (kKeyIDMaskExtended | 0x000c)
#define KEYID_LOCK_NUM (kKeyIDMaskExtended | 0x000d)
#define KEYID_LOCK_SCROOL (kKeyIDMaskExtended | 0x000e)
#define KEYID_LOCK_CAPS (kKeyIDMaskExtended | 0x000f)
#define KEYID_SHIFT_L (kKeyIDMaskExtended | 0x0010)
#define KEYID_SHIFT_R (kKeyIDMaskExtended | 0x0011)
#define KEYID_CTRL_L (kKeyIDMaskExtended | 0x0012)
#define KEYID_CTRL_R (kKeyIDMaskExtended | 0x0013)
#define KEYID_ALT_L (kKeyIDMaskExtended | 0x0014)
#define KEYID_ALT_R (kKeyIDMaskExtended | 0x0015)
#define KEYID_DELETE (kKeyIDMaskExtended | 0x0016)
#define KEYID_INSERT (kKeyIDMaskExtended | 0x0017)
#define KEYID_PAUSE (kKeyIDMaskExtended | 0x0018)
#define KEYID_BREAK (kKeyIDMaskExtended | 0x0019)
#define KEYID_PRINT_SCREEN (kKeyIDMaskExtended | 0x001a)
#define KEYID_SYS_RQ (kKeyIDMaskExtended | 0x001b)
#define KEYID_CURSOR_U (kKeyIDMaskExtended | 0x001c)
#define KEYID_CURSOR_D (kKeyIDMaskExtended | 0x001d)
#define KEYID_CURSOR_L (kKeyIDMaskExtended | 0x001e)
#define KEYID_CURSOR_R (kKeyIDMaskExtended | 0x001f)
#define KEYID_PAGE_UP (kKeyIDMaskExtended | 0x0020)
#define KEYID_PAGE_DOWN (kKeyIDMaskExtended | 0x0021)
#define KEYID_HOME (kKeyIDMaskExtended | 0x0022)
#define KEYID_END (kKeyIDMaskExtended | 0x0023)
#define KEYID_ICON_L (kKeyIDMaskExtended | 0x0024)
#define KEYID_ICON_R (kKeyIDMaskExtended | 0x0025)
#define KEYID_MENU (kKeyIDMaskExtended | 0x0026)
#define KEYID_KANJI (kKeyIDMaskExtended | 0x0027)
#define KEYID_HIRAGANA (kKeyIDMaskExtended | 0x0028)
#define KEYID_HENKAN (kKeyIDMaskExtended | 0x0029)
#define KEYID_MUHENKAN (kKeyIDMaskExtended | 0x002a)

#define KEYID_BACKSPACE (kKeyIDMaskExtended | 0x0040)
#define KEYID_TAB (kKeyIDMaskExtended | 0x0041)
#define KEYID_ENTER (kKeyIDMaskExtended | 0x0042)

#define KEYID_KBD_ERROR (kKeyIDMaskExtended | 0x007e)
#define KEYID_UNKNOWN (kKeyIDMaskExtended | 0x007f)

static const uint16_t kKeyCodeTable[0x80] = {
    0, KEYID_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^',
    KEYID_BACKSPACE, KEYID_TAB,
    /*0x10*/
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', KEYID_ENTER,
    KEYID_CTRL_L, 'A', 'S',
    /*0x20*/
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', KEYID_KANJI, KEYID_SHIFT_L,
    ']', 'Z', 'X', 'C', 'V',
    /*0x30*/
    'B', 'N', 'M', ',', '.', '/', KEYID_SHIFT_R, KEYID_MASK_TENKEY | '*',
    KEYID_ALT_L, ' ', KEYID_LOCK_CAPS, KEYID_F1, KEYID_F2, KEYID_F3, KEYID_F4,
    KEYID_F5,
    /*0x40*/
    KEYID_F6, KEYID_F7, KEYID_F8, KEYID_F9, KEYID_F10, KEYID_LOCK_NUM,
    KEYID_LOCK_SCROOL, KEYID_MASK_TENKEY | '7', KEYID_MASK_TENKEY | '8',
    KEYID_MASK_TENKEY | '9', KEYID_MASK_TENKEY | '-', KEYID_MASK_TENKEY | '4',
    KEYID_MASK_TENKEY | '5', KEYID_MASK_TENKEY | '6', KEYID_MASK_TENKEY | '+',
    KEYID_MASK_TENKEY | '1',
    /*0x50*/
    KEYID_MASK_TENKEY | '2', KEYID_MASK_TENKEY | '3', KEYID_MASK_TENKEY | '0',
    KEYID_MASK_TENKEY | '.', KEYID_SYS_RQ, 0x0000, 0x0000, KEYID_F11, KEYID_F12,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x60*/
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x70*/
    KEYID_HIRAGANA, 0x0000, 0x0000, '_', 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    KEYID_HENKAN, 0x0000, KEYID_MUHENKAN, 0x0000, '\\',
    //  0x5c, // ='\' for mikan-trap.
    0x0000, 0x0000};

static const uint16_t kKeyCodeTableWithShift[0x80] = {
    0x00, 0x00, '!', '"', '#', '$', '%', '&', '\'', '(', ')', '~', '=', '~',
    0x00, 0x00,
    /*0x10*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '`', '{', 0x00,
    0x00, 0x00, 0x00,
    /*0x20*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '+', '*', 0x00, 0x00, '}', 0x00,
    0x00, 0x00, 0x00,
    /*0x30*/
    0x00, 0x00, 0x00, '<', '>', '?', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    /*0x40*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /*0x50*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /*0x60*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    /*0x70*/
    0x00, 0x00, 0x00, '_', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    '|', 0x00, 0x00};

static bool state_shift;

constexpr uint16_t kKeyScanCodeMaskBreak = 0x80;

uint16_t ParseKeyCode(uint8_t keycode) {
  uint16_t keyid;
  if (state_shift) {
    keyid = kKeyCodeTableWithShift[keycode];
    if (!keyid)
      keyid = kKeyCodeTable[keycode];
  } else {
    keyid = kKeyCodeTable[keycode];
  }

  if ('A' <= keyid && keyid <= 'Z' && !state_shift)
    keyid += 0x20;
  if (!keyid)
    return 0;

  if (keycode & kKeyScanCodeMaskBreak) {
    if (keyid == KEYID_SHIFT_L || keyid == KEYID_SHIFT_R)
      state_shift = false;
    keyid |= kKeyIDMaskBreak;
  } else {
    if (keyid == KEYID_SHIFT_L || keyid == KEYID_SHIFT_R)
      state_shift = true;
  }
  return keyid;
}
