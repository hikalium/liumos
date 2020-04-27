#include "adlib.h"

#include <stdio.h>
#include <string.h>

int Adlib::note_on_count_[0x100];
int Adlib::ch_of_note_[0x100];
int Adlib::note_of_ch_[9];

unsigned int ParseMIDIValue(const uint8_t* base, unsigned int* offset) {
  unsigned int i, r;

  r = 0;

  for (i = 0; i < 4; i++) {
    r = r << 7;
    r += (base[*offset + i] & 0x7f);
    if ((base[*offset + i] & 0x80) == 0) {
      i++;
      break;
    }
  }

  *offset += i;

  return r;
}

const char* MIDI_Notes[12] = {"C ", "C#", "D ", "D#", "E ", "F ",
                              "F#", "G ", "G#", "A ", "A#", "B "};

void PlayMIDI(EFIFile& file) {
  if (!Adlib::DetectAndInit()) {
    return;
  }

  const uint8_t* data = file.GetBuf();
  unsigned int data_size = static_cast<unsigned int>(file.GetFileSize());
  unsigned int p, q, tracksize, r, datalength, delta;
  char s[128];
  unsigned int DeltaTimePerQuarterNote, microsTimePerQuarterNote,
      microsTimePerDeltaTime;

  DeltaTimePerQuarterNote = 480;
  microsTimePerQuarterNote = 500000;
  microsTimePerDeltaTime = microsTimePerQuarterNote / DeltaTimePerQuarterNote;
  p = 0;
  if (strncmp(reinterpret_cast<const char*>(&data[p + 0]), "MThd", 4) == 0) {
    p += 4;
    p += 4;

    sprintf(s, "SMF Format%d ", (data[p + 0] << 8) | data[p + 1]);
    PutString(s);
    p += 2;

    sprintf(s, "Tracks:%d\n", (data[p + 0] << 8) | data[p + 1]);
    PutString(s);
    p += 2;

    DeltaTimePerQuarterNote = (data[p + 0] << 8) | data[p + 1];
    microsTimePerDeltaTime = microsTimePerQuarterNote / DeltaTimePerQuarterNote;
    sprintf(s, "DeltaTime(per quarter note):%d\n", DeltaTimePerQuarterNote);
    PutString(s);
    p += 2;

    for (; p < data_size;) {
      if (strncmp(reinterpret_cast<const char*>(&data[p + 0]), "MTrk", 4) ==
          0) {
        PutString("Track:\n");
        p += 4;

        tracksize = (data[p + 0] << 24) | (data[p + 1] << 16) |
                    (data[p + 2] << 8) | data[p + 3];
        sprintf(s, "size:0x%X\n", tracksize);
        PutString(s);
        p += 4;

        for (q = 0; q < tracksize;) {
          delta = ParseMIDIValue(&data[p + 0], &q);
          sprintf(s, "%8d:", delta);
          PutString(s);

          if (delta)
            liumos->hpet->BusyWaitMicroSecond(microsTimePerDeltaTime * delta);

          if (data[p + q + 0] == 0xff) {
            PutString("Meta ");
            q++;
            r = data[p + q + 0];
            q++;
            datalength = ParseMIDIValue(&data[p + 0], &q);
            if (r == 0x00) {  //シーケンス番号
              sprintf(s, "SequenceNumber size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x01) {  //テキスト
              sprintf(s, "TEXT size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x02) {  //著作権表示
              sprintf(s, "Copyright size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x03) {  //トラック名
              sprintf(s, "TrackName size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x04) {  //楽器名
              sprintf(s, "InstrumentName size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x05) {  //歌詞
              sprintf(s, "Lyrics size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x06) {  //マーカー
              sprintf(s, "Marker size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x07) {  //キューポイント
              sprintf(s, "QueuePoint size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x08) {  //音色（プログラム）名
              sprintf(s, "ProgramName size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x09) {  //音源（デバイス）名
              sprintf(s, "DeviceName size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x20) {  // MIDIチャンネルプリフィックス
              sprintf(s, "MIDIChannelPrefix size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x21) {  //ポート指定
              sprintf(s, "OutPutPort size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x2f) {  // End of Track
              sprintf(s, "End of Track size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x51) {  //テンポ設定（4分音符あたりのマイクロ秒）
              microsTimePerQuarterNote = (data[p + q + 0] << 16) |
                                         (data[p + q + 1] << 8) |
                                         data[p + q + 2];
              sprintf(s, "Tempo %dmicrosec per quarter note.\n", datalength);
              microsTimePerDeltaTime =
                  microsTimePerQuarterNote / DeltaTimePerQuarterNote;
              PutString(s);
              q += datalength;
            } else if (r == 0x54) {  // SMPTEオフセット
              sprintf(s, "SMPTE_Offset size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x58) {  //拍子の設定
              sprintf(s, "Beat size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x59) {  //調の設定
              sprintf(s, "Tone size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else if (r == 0x7f) {  //シーケンサ特定メタイベント
              sprintf(s, "SequencerSpecialMetaEvent size:%d\n", datalength);
              PutString(s);
              q += datalength;
            } else {
              PutString("midi:Unknown Meta Event.\n");
              break;
            }
          } else if ((data[p + q + 0] & 0xf0) == 0x80) {  // note off
            Adlib::NoteOff(data[p + q + 1]);
            q += 3;
          } else if ((data[p + q + 0] & 0xf0) == 0x90) {  // note on or off
            if (data[p + q + 2] == 0x00) {                // note off
              Adlib::NoteOff(data[p + q + 1]);
            } else {
              Adlib::NoteOn(data[p + q + 1]);
            }
            q += 3;
          } else if ((data[p + q + 0] & 0xf0) ==
                     0xa0) {  // Polyphonic Key Pressure
            q += 3;
          } else if ((data[p + q + 0] & 0xf0) == 0xb0) {  // Control Change
            q += 3;
          } else if ((data[p + q + 0] & 0xf0) == 0xc0) {  // Program Change
            q += 2;
          } else if ((data[p + q + 0] & 0xf0) == 0xd0) {  // Channel Pressure
            q += 2;
          } else if ((data[p + q + 0] & 0xf0) == 0xe0) {  // Pitch Bend
            q += 3;
          } else {
            break;
          }
        }
        p += tracksize;
      } else {
        PutString("midi:Unknown Track.\n");
      }
    }
  } else {
    PutString("midi:Unknown header.\n");
  }
  Adlib::TurnOffAllNotes();
}

void TestAdlib() {
  if (!Adlib::DetectAndInit()) {
    return;
  }

  Adlib::NoteOn(60);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(60);

  Adlib::NoteOn(64);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(64);

  Adlib::NoteOn(67);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(67);

  liumos->hpet->BusyWait(500);

  Adlib::NoteOn(60);
  Adlib::NoteOn(64);
  Adlib::NoteOn(67);
  liumos->hpet->BusyWait(500);
  Adlib::NoteOff(60);
  Adlib::NoteOff(64);
  Adlib::NoteOff(67);

  liumos->hpet->BusyWait(500);

  for (int i = 0; i < 10; i++) {
    Adlib::NoteOn(i + 60);
    liumos->hpet->BusyWait(200);
  }
  for (int i = 0; i < 10; i++) {
    liumos->hpet->BusyWait(200);
    Adlib::NoteOff(i + 60);
  }

  Adlib::TurnOffAllNotes();
}
