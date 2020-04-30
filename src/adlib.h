#pragma once

#include "liumos.h"

class Adlib {
 private:
  static uint8_t ReadStatusReg() { return ReadIOPort8(0x388) & 0xE0; }
  static void WriteReg(uint8_t addr, uint8_t data) {
    //   The most accurate method of producing the delay is to read the register
    //   port six times after writing to the register port, and read the
    //   register port thirty-five times after writing to the data port.
    WriteIOPort8(0x388, addr);
    for (int i = 0; i < 6; i++) {
      ReadStatusReg();
    }
    WriteIOPort8(0x389, data);
    for (int i = 0; i < 35; i++) {
      ReadStatusReg();
    }
  }
  static void ResetAllRegisters() {
    for (int i = 0x01; i <= 0xF5; i++) {
      WriteReg(i, 0);
    }
  }
  static void SetupChannel(int ch) {
    // volume is 6bit (0 - 63)
    // Channel        0   1   2   3   4   5   6   7   8
    // Operator 1    00  01  02  08  09  0A  10  11  12
    // Operator 2    03  04  05  0B  0C  0D  13  14  15
    int op1ofs = (ch / 3) * 8 + ch % 3;
    int op2ofs = (ch / 3) * 8 + ch % 3 + 3;

    //        20          01      Set the modulator's multiple to 1
    WriteReg(0x20 + op1ofs, 0x01);
    //        40          10      Set the modulator's level to about 40 dB
    WriteReg(0x40 + op1ofs, 0b0001'0000);
    //        60          F0      Modulator attack:  quick;   decay:   long
    WriteReg(0x60 + op1ofs, 0xF0);
    //        80          77      Modulator sustain: medium;  release: medium
    WriteReg(0x80 + op1ofs, 0x77);
    //        23          01      Set the carrier's multiple to 1
    WriteReg(0x20 + op2ofs, 0x01);
    //        43          00      Set the carrier to maximum volume (about 47
    //        dB)
    SetVolume(ch, 63);
    //        63          F0      Carrier attack:  quick;   decay:   long
    WriteReg(0x60 + op2ofs, 0xF0);
    //        83          77      Carrier sustain: medium;  release: medium
    WriteReg(0x80 + op2ofs, 0x77);
  }
  static void PrintNote(const char* label, int midi_note_num) {
    static const char* freq_str[12] = {"C#", "D",  "D#", "E",  "F", "F#",
                                       "G",  "G#", "A",  "A#", "B", "C"};
    int ofs = (midi_note_num - 13) % 12;
    PutString(label);
    PutStringAndHex(freq_str[ofs], midi_note_num);
  }
  static void SetFreq(int ch, uint8_t octave, uint16_t freq_num) {
    // Set following registers:
    // A0..A8       Frequency (low 8 bits)
    // B0..B8       Key On / Octave / Frequency (high 2 bits)
    assert(0 <= ch && ch <= 8);
    //        A0          98      Set voice frequency's LSB (it'll be a D#)
    WriteReg(0xA0 + ch, freq_num & 0xFF);
    //        B0          31      Turn the voice on; set the octave and freq MSB
    WriteReg(0xB0 + ch, 0b0010'0000 | (octave << 2) | freq_num >> 8);
  }
  static void SetFreq(int ch, int midi_note_num) {
    // http://bochs.sourceforge.net/techspec/adlib_sb.txt
    static uint16_t freq_array[12] = {0x16B, 0x181, 0x198, 0x1B0, 0x1CA, 0x1E5,
                                      0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE};
    assert(13 <= midi_note_num && midi_note_num <= 108);
    int ofs = (midi_note_num - 13) % 12;
    SetFreq(ch, (midi_note_num - 13) / 12, freq_array[ofs]);
  }
  static void SetVolume(int ch, uint8_t volume) {
    int op2ofs = (ch / 3) * 8 + ch % 3 + 3;
    WriteReg(0x40 + op2ofs, 0b0011'1111 - (volume & 0b0011'1111));
  }
  static void StopSound(int ch) { WriteReg(0xB0 + ch, 0); }

  static constexpr int kChNone = -1;
  static constexpr int kNoteNone = -1;
  static int note_on_count_[0x100];
  static int ch_of_note_[0x100];
  static int note_of_ch_[9];

 public:
  static bool DetectAndInit() {
    // returns true if adlib exists.
    PutString("Test \n");
    //      1)  Reset both timers by writing 60h to register 4.
    WriteReg(4, 0x60);
    //      2)  Enable the interrupts by writing 80h to register 4.  NOTE: this
    //          must be a separate step from number 1.
    WriteReg(4, 0x80);
    //      3)  Read the status register (port 388h).  Store the result.
    uint8_t result0 = ReadStatusReg();
    //      4)  Write FFh to register 2 (Timer 1).
    WriteReg(2, 0xFF);
    //      5)  Start timer 1 by writing 21h to register 4.
    WriteReg(4, 0x21);
    //      6)  Delay for at least 80 microseconds.
    //      7)  Read the status register (port 388h).  Store the result.
    uint8_t result1 = ReadStatusReg();
    //      8)  Reset both timers and interrupts (see steps 1 and 2).
    WriteReg(4, 0x60);
    //      9)  Test the stored results of steps 3 and 7 by ANDing them
    //          with E0h.  The result of step 3 should be 00h, and the
    //          result of step 7 should be C0h.  If both are correct, an
    //          AdLib-compatible board is installed in the computer.
    if (result0 != 0x00 || result1 != 0xC0) {
      PutString(" not found.\n");
      return false;
    }
    PutString(" found.\n");
    ResetAllRegisters();
    TurnOffAllNotes();
    for (int i = 0; i < 9; i++) {
      SetupChannel(i);
    }
    return true;
  }
  static void TurnOffAllNotes() {
    for (int i = 0; i < 9; i++) {
      StopSound(i);
      note_of_ch_[i] = kNoteNone;
    }
    for (int i = 0; i < 0x100; i++) {
      note_on_count_[i] = 0;
      ch_of_note_[i] = kChNone;
    }
  }

  static void NoteOn(int midi_note_num) {
    assert(13 <= midi_note_num && midi_note_num <= 108);
    if (note_on_count_[midi_note_num]) {
      // Already on.
      note_on_count_[midi_note_num]++;
      return;
    }
    for (int i = 0; i < 9; i++) {
      if (note_of_ch_[i] != kNoteNone)
        continue;
      note_of_ch_[i] = midi_note_num;
      note_on_count_[midi_note_num] = 1;
      ch_of_note_[midi_note_num] = i;
      SetFreq(i, midi_note_num);
      // PrintNote("Note on : ", midi_note_num);
      return;
    }
    // Give up since we have no more channel.
  }

  static void NoteOff(int midi_note_num) {
    assert(13 <= midi_note_num && midi_note_num <= 108);
    if (!note_on_count_[midi_note_num]) {
      // Already off.
      return;
    }
    note_on_count_[midi_note_num]--;
    if (note_on_count_[midi_note_num]) {
      // Keep note on
      return;
    }
    StopSound(ch_of_note_[midi_note_num]);
    // PrintNote("Note off: ", midi_note_num);
    note_of_ch_[ch_of_note_[midi_note_num]] = kNoteNone;
    ch_of_note_[midi_note_num] = kChNone;
  }
};

void TestAdlib();
void PlayMIDI(EFIFile& file);
