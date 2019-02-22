#include "liumos.h"

void TextBox::putc(uint16_t keyid) {
  if (keyid & KeyID::kMaskBreak)
    return;
  if (keyid == KeyID::kBackspace) {
    if (is_recording_enabled_) {
      if (buf_used_ == 0)
        return;
      buf_[--buf_used_] = 0;
    }
    PutChar('\b');
    return;
  }
  if (keyid & KeyID::kMaskExtended)
    return;
  if (is_recording_enabled_) {
    if (buf_used_ >= kSizeOfBuffer)
      return;
    buf_[buf_used_++] = (uint8_t)keyid;
    buf_[buf_used_] = 0;
  }
  PutChar(keyid);
}
