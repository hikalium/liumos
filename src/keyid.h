#pragma once

// KeyID:
// BX..'..CS'cccc'cccc
//           ^^^^ ^^^^ : char code (if X=0) or key code(X=1)
//         ^           : Shift is pressed or not
//        ^            : Ctrl is pressed or not
//  ^                  : Extended(1) or char(0)
// ^                   : Make(0) or Break(1)

namespace KeyID {
constexpr uint16_t kMaskBreak = 0b1000'0000'0000'0000;
constexpr uint16_t kMaskExtended = 0b0100'0000'0000'0000;
constexpr uint16_t kMaskCode = 0b0000'0000'1111'1111;
constexpr uint16_t kMaskAttr = 0b0000'0011'0000'0000;
constexpr uint16_t kAttrShift = 0b0000'0001'0000'0000;
constexpr uint16_t kAttrCtrl = 0b0000'0010'0000'0000;
constexpr uint16_t kEsc = KeyID::kMaskExtended | 0x0000;
constexpr uint16_t kF1 = KeyID::kMaskExtended | 0x0001;
constexpr uint16_t kF2 = KeyID::kMaskExtended | 0x0002;
constexpr uint16_t kF3 = KeyID::kMaskExtended | 0x0003;
constexpr uint16_t kF4 = KeyID::kMaskExtended | 0x0004;
constexpr uint16_t kF5 = KeyID::kMaskExtended | 0x0005;
constexpr uint16_t kF6 = KeyID::kMaskExtended | 0x0006;
constexpr uint16_t kF7 = KeyID::kMaskExtended | 0x0007;
constexpr uint16_t kF8 = KeyID::kMaskExtended | 0x0008;
constexpr uint16_t kF9 = KeyID::kMaskExtended | 0x0009;
constexpr uint16_t kF10 = KeyID::kMaskExtended | 0x000a;
constexpr uint16_t kF11 = KeyID::kMaskExtended | 0x000b;
constexpr uint16_t kF12 = KeyID::kMaskExtended | 0x000c;
constexpr uint16_t kNumLock = KeyID::kMaskExtended | 0x000d;
constexpr uint16_t kScrollLock = KeyID::kMaskExtended | 0x000e;
constexpr uint16_t kCapsLock = KeyID::kMaskExtended | 0x000f;
constexpr uint16_t kShiftL = KeyID::kMaskExtended | 0x0010;
constexpr uint16_t kShiftR = KeyID::kMaskExtended | 0x0011;
constexpr uint16_t kCtrlL = KeyID::kMaskExtended | 0x0012;
constexpr uint16_t kCtrlR = KeyID::kMaskExtended | 0x0013;
constexpr uint16_t kAltL = KeyID::kMaskExtended | 0x0014;
constexpr uint16_t kAltR = KeyID::kMaskExtended | 0x0015;
constexpr uint16_t kDelete = KeyID::kMaskExtended | 0x0016;
constexpr uint16_t kInsert = KeyID::kMaskExtended | 0x0017;
constexpr uint16_t kPause = KeyID::kMaskExtended | 0x0018;
constexpr uint16_t kBreak = KeyID::kMaskExtended | 0x0019;
constexpr uint16_t kPrintScreen = KeyID::kMaskExtended | 0x001a;
constexpr uint16_t kSysRq = KeyID::kMaskExtended | 0x001b;
constexpr uint16_t kCursorUp = KeyID::kMaskExtended | 0x001c;
constexpr uint16_t kCursorDown = KeyID::kMaskExtended | 0x001d;
constexpr uint16_t kCursorLeft = KeyID::kMaskExtended | 0x001e;
constexpr uint16_t kCursorRight = KeyID::kMaskExtended | 0x001f;
constexpr uint16_t kPageUp = KeyID::kMaskExtended | 0x0020;
constexpr uint16_t kPageDown = KeyID::kMaskExtended | 0x0021;
constexpr uint16_t kHome = KeyID::kMaskExtended | 0x0022;
constexpr uint16_t kEnd = KeyID::kMaskExtended | 0x0023;
constexpr uint16_t kCmdL = KeyID::kMaskExtended | 0x0024;
constexpr uint16_t kCmdR = KeyID::kMaskExtended | 0x0025;
constexpr uint16_t kMenu = KeyID::kMaskExtended | 0x0026;
constexpr uint16_t kKanji = KeyID::kMaskExtended | 0x0027;
constexpr uint16_t kHiragana = KeyID::kMaskExtended | 0x0028;
constexpr uint16_t kHenkan = KeyID::kMaskExtended | 0x0029;
constexpr uint16_t kMuhenkan = KeyID::kMaskExtended | 0x002a;

constexpr uint16_t kBackspace = KeyID::kMaskExtended | 0x0040;
constexpr uint16_t kTab = KeyID::kMaskExtended | 0x0041;
constexpr uint16_t kEnter = KeyID::kMaskExtended | 0x0042;

constexpr uint16_t kError = KeyID::kMaskExtended | 0x007e;
constexpr uint16_t kUnknown = KeyID::kMaskExtended | 0x007f;
constexpr uint16_t kNoInput = KeyID::kMaskExtended | 0x00ff;
constexpr bool IsWithShift(uint16_t k) {
  return k & kAttrShift;
}
constexpr bool IsWithCtrl(uint16_t k) {
  return k & kAttrCtrl;
}
constexpr bool IsChar(uint16_t k, char c) {
  return !(k & kMaskExtended) && ((k & kMaskCode) == c);
}
constexpr uint16_t WithoutAttr(uint16_t k) {
  return k & ~kMaskAttr;
}
}  // namespace KeyID
