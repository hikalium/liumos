#include "liumos.h"

#define KEYID_MASK_ID 0x007f
#define KEYID_MASK_TENKEY 0x0100
#define KEYID_MASK_STATE_SHIFT 0x0200
#define KEYID_MASK_STATE_CTRL 0x0400
#define KEYID_MASK_STATE_ALT 0x0800
#define KEYID_MASK_STATE_LOCK_SCROOL 0x1000
#define KEYID_MASK_STATE_LOCK_NUM 0x2000
#define KEYID_MASK_STATE_LOCK_CAPS 0x4000

using namespace KeyID;

KeyboardController* KeyboardController::last_instance_;

static const uint16_t kKeyCodeTable[0x80] = {
    0, kEsc, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^',
    kBackspace, kTab,
    /*0x10*/
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', kEnter, kCtrlL,
    'A', 'S',
    /*0x20*/
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', kKanji, kShiftL, ']', 'Z', 'X',
    'C', 'V',
    /*0x30*/
    'B', 'N', 'M', ',', '.', '/', kShiftR, KEYID_MASK_TENKEY | '*', kAltL, ' ',
    kCapsLock, kF1, kF2, kF3, kF4, kF5,
    /*0x40*/
    kF6, kF7, kF8, kF9, kF10, kNumLock, kScrollLock, KEYID_MASK_TENKEY | '7',
    KEYID_MASK_TENKEY | '8', KEYID_MASK_TENKEY | '9', KEYID_MASK_TENKEY | '-',
    KEYID_MASK_TENKEY | '4', KEYID_MASK_TENKEY | '5', KEYID_MASK_TENKEY | '6',
    KEYID_MASK_TENKEY | '+', KEYID_MASK_TENKEY | '1',
    /*0x50*/
    KEYID_MASK_TENKEY | '2', KEYID_MASK_TENKEY | '3', KEYID_MASK_TENKEY | '0',
    KEYID_MASK_TENKEY | '.', kSysRq, 0x0000, 0x0000, kF11, kF12, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x60*/
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x70*/
    kHiragana, 0x0000, 0x0000, '_', 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    kHenkan, 0x0000, kMuhenkan, 0x0000, '\\',
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

constexpr uint8_t kKeyScanCodeMaskBreak = 0x80;

void KeyboardController::Init() {
  state_shift_ = false;
  new (&keycode_buffer_) RingBuffer<uint8_t, 16>();
  last_instance_ = this;
  liumos->idt->SetIntHandler(0x21, KeyboardController::IntHandler);
  liumos->keyboard_ctrl = this;
}

void KeyboardController::IntHandlerSub(uint64_t, InterruptInfo*) {
  keycode_buffer_.Push(ReadIOPort8(kIOPortKeyboardData));
  liumos->bsp_local_apic->SendEndOfInterrupt();
}

uint16_t KeyboardController::ParseKeyCode(uint8_t keycode) {
  bool is_break = keycode & kKeyScanCodeMaskBreak;
  keycode &= ~kKeyScanCodeMaskBreak;
  uint16_t keyid;
  if (state_shift_) {
    keyid = kKeyCodeTableWithShift[keycode];
    if (!keyid)
      keyid = kKeyCodeTable[keycode];
  } else {
    keyid = kKeyCodeTable[keycode];
  }

  if (!keyid)
    return 0;

  if ('A' <= keyid && keyid <= 'Z' && !state_shift_)
    keyid += 0x20;

  if (is_break)
    keyid |= kMaskBreak;

  switch (keycode) {
    case kShiftL:
    case kShiftR:
      state_shift_ = true;
      break;
    case kShiftL | kMaskBreak:
    case kShiftR | kMaskBreak:
      state_shift_ = false;
      break;
  }
  return keyid;
}
