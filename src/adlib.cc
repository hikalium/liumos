#include "adlib.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>

int Adlib::note_on_count_[0x100];
int Adlib::ch_of_note_[0x100];
int Adlib::note_of_ch_[9];

unsigned int ParseMIDIValue(const uint8_t* base, int* offset) {
  unsigned int r;
  int i;
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

struct MIDIPlayerState {
  unsigned int delta_per_quarter_note;
  unsigned int micro_second_per_quarter_note;
  unsigned int micro_second_per_delta;
};

class MIDIEvent {
 public:
  bool Init(const uint8_t* data) {
    if (!data)
      return false;
    data_ = data;
    int ofs = 0;
    delta_ = ParseMIDIValue(data, &ofs);
    type_ = data[ofs++];
    param_ofs_ = ofs;
    if (type_ == 0xff) {
      // Meta Event
      meta_type_ = data[ofs++];
      meta_data_length_ = ParseMIDIValue(data, &ofs);
      meta_data_ofs_ = ofs;
      event_size_ = ofs + meta_data_length_;
      return true;
    }
    switch (type_ & 0xf0) {
      case 0x80:
      case 0x90:
      case 0xa0:
      case 0xb0:
      case 0xe0:
        event_size_ = ofs + 2;
        return true;
      case 0xc0:
      case 0xd0:
        event_size_ = ofs + 1;
        return true;
    }
    return false;
  }
  int GetSize() const { return event_size_; };
  int GetDelta() const { return delta_; }
  int GetType() const { return type_; }
  int GetMetaType() const { return meta_type_; }
  int GetParam(int index) const {
    assert(0 <= index && index < (event_size_ - param_ofs_));
    return data_[param_ofs_ + index];
  }
  int GetMetaData(int index) const {
    assert(0 <= index && index < (event_size_ - meta_data_ofs_));
    return data_[meta_data_ofs_ + index];
  }

 private:
  const uint8_t* data_;
  int delta_;
  int param_ofs_;
  int meta_data_ofs_;
  uint8_t type_;
  uint8_t meta_type_;
  uint8_t meta_data_length_;
  uint8_t event_size_;
};

class MIDITrackStream {
 public:
  bool Init(const uint8_t* data, int data_size, int ofs) {
    // returns the parameter is valid or not
    data_ = data;
    data_size_ = data_size;
    ofs_base_ = ofs;
    ofs_ = ofs_base_;
    if (strncmp(reinterpret_cast<const char*>(&data_[ofs_]), "MTrk", 4) != 0) {
      PutString("Not a Track.\n");
      return false;
    }
    ofs_ += 4;

    int track_data_size = 0;
    for (int i = 0; i < 4; i++) {
      track_data_size <<= 8;
      track_data_size |= data_[ofs_ + i];
    }
    track_size_ = track_data_size + 4 + 4;
    ofs_ += 4;

    char s[128];
    sprintf(s, "Track size=%d ofs_base=%d ofs=%d\n", track_size_, ofs_base_,
            ofs_);
    PutString(s);
    assert(data_size_ - ofs_base_ >= track_size_);
    return true;
  }
  int GetSize() { return track_size_; }
  const MIDIEvent* GetNextEvent() {
    if (ofs_ >= (ofs_base_ + track_size_))
      return nullptr;
    if (!event_.Init(&data_[ofs_]))
      return nullptr;
    ofs_ += event_.GetSize();
    return &event_;
  }

 private:
  MIDIEvent event_;
  const uint8_t* data_;
  int data_size_;
  int ofs_base_;
  int track_size_;
  int ofs_;
};

void PlayMIDI(EFIFile& file) {
  char s[128];
  if (!Adlib::DetectAndInit()) {
    return;
  }
  const uint8_t* data = file.GetBuf();
  int data_size = static_cast<int>(file.GetFileSize());
  sprintf(s, "data_size=%d ", data_size);
  PutString(s);

  int p;
  MIDIPlayerState state;

  p = 0;
  if (strncmp(reinterpret_cast<const char*>(&data[p + 0]), "MThd", 4) != 0) {
    PutString("midi:Unknown header.\n");
    return;
  }
  p += 4;
  p += 4;

  sprintf(s, "SMF Format%d ", (data[p + 0] << 8) | data[p + 1]);
  PutString(s);
  p += 2;

  int num_of_tracks = (data[p + 0] << 8) | data[p + 1];
  sprintf(s, "Tracks:%d\n", num_of_tracks);
  PutString(s);
  p += 2;

  state.micro_second_per_quarter_note = 500000;
  state.delta_per_quarter_note = (data[p + 0] << 8) | data[p + 1];
  state.micro_second_per_delta =
      state.micro_second_per_quarter_note / state.delta_per_quarter_note;
  sprintf(s, "DeltaTime(per quarter note):%d\n", state.delta_per_quarter_note);
  PutString(s);
  p += 2;

  constexpr int kMaxNumOfTracks = 24;
  MIDITrackStream tracks[kMaxNumOfTracks];
  const MIDIEvent* next_event[kMaxNumOfTracks];
  assert(num_of_tracks < kMaxNumOfTracks);
  int delta_used[kMaxNumOfTracks];

  for (int i = 0; i < num_of_tracks; i++) {
    assert(p < data_size);
    assert(tracks[i].Init(data, data_size, p));
    next_event[i] = tracks[i].GetNextEvent();
    delta_used[i] = 0;
    p += tracks[i].GetSize();
  }

  bool has_next_event = true;
  while (has_next_event) {
    for (int i = 0; i < num_of_tracks; i++) {
      while (next_event[i]) {
        const MIDIEvent& event = *next_event[i];
        if (event.GetDelta() - delta_used[i] > 0)
          break;
        if ((event.GetType() & 0xf0) == 0x80) {
          // Note Off
          // 0b1000'nnnn 0b0kkk'kkkk 0b0vvv'vvvv
          // n: port number
          // k: note number
          // v: velocity
          Adlib::NoteOff(event.GetParam(0));
        } else if ((event.GetType() & 0xf0) == 0x90) {  // note on or off
          if (event.GetParam(1) == 0x00) {              // note off
            Adlib::NoteOff(event.GetParam(0));
          } else {
            Adlib::NoteOn(event.GetParam(0));
          }
        } else if ((event.GetType() == 0xff)) {
          // Meta
          if (event.GetMetaType() == 0x51) {
            // Tempo (micro sec per quarter note)
            state.micro_second_per_quarter_note = (event.GetMetaData(0) << 16) |
                                                  (event.GetMetaData(1) << 8) |
                                                  event.GetMetaData(2);
            sprintf(s, "Tempo %d micro sec per quarter note.\n",
                    state.micro_second_per_quarter_note);
            PutString(s);
            state.micro_second_per_delta = state.micro_second_per_quarter_note /
                                           state.delta_per_quarter_note;
          }
        }
        next_event[i] = tracks[i].GetNextEvent();
        delta_used[i] = 0;
      }
    }
    // Determine min delta
    has_next_event = false;
    int min_delta = 0;
    for (int i = 0; i < num_of_tracks; i++) {
      if (!next_event[i])
        continue;
      if (!has_next_event) {
        min_delta = next_event[i]->GetDelta() - delta_used[i];
      } else {
        min_delta =
            std::min(min_delta, next_event[i]->GetDelta() - delta_used[i]);
      }
      has_next_event = true;
    }
    if (min_delta != 0) {
      liumos->hpet->BusyWaitMicroSecond(state.micro_second_per_delta *
                                        min_delta);
    }
    for (int i = 0; i < num_of_tracks; i++) {
      delta_used[i] += min_delta;
    }
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
