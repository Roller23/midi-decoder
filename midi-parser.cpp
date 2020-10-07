#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>

#include "midi-parser.hpp"

const midiParser::note_meta midiParser::notes_data[12] = {
  {"B", 61.74}, {"C", 32.70}, {"C#", 34.65}, {"D", 36.71},
  {"D#", 38.89},{"E", 41.20}, {"F", 43.65}, {"F#", 46.25},
  {"G", 49.00}, {"G#", 51.91}, {"A", 55.00}, {"A#", 58.27}
};

std::uint16_t midiParser::bswap(std::uint16_t x) {
  return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

std::uint32_t midiParser::bswap(std::uint32_t x) {
  x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
  return ((x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8);
}

midiParser::midiParser(std::string midi_filename) {
  this->midi_file.open(midi_filename, std::ios::binary);
  this->midi_file.seekg(0, this->midi_file.beg);
  for (int i = 0; i < 50; i++) {
    std::uint8_t byte = 0;
    this->midi_file >> byte;
    std::printf("%.2x ", byte);
  }
  this->midi_file.seekg(0, this->midi_file.beg);
}

midiParser::~midiParser() {
  this->midi_file.close();
}

void midiParser::read_chunk(chunk *_chunk) {
  std::memset(_chunk, 0, sizeof(chunk));
  this->midi_file.read((char *)_chunk->chunk_meta, CHUNK_META_LENGTH);
  _chunk->size = bswap(_chunk->size);
  _chunk->data = new std::uint8_t[_chunk->size];
  this->midi_file.read((char *)_chunk->data, _chunk->size);
  std::printf("Chunk name: '%.4s'\n", _chunk->name);
  std::cout << "Chunk size: " << _chunk->size << "\n";
}

void midiParser::free_chunk(chunk *_chunk) {
  delete _chunk->data;
}

std::uint8_t midiParser::read_byte(std::uint8_t **data) {
  std::uint8_t byte = **data;
  (*data)++;
  return byte;
}

char *midiParser::read_string(std::uint8_t **data, std::uint8_t length) {
  char *string = new char[length + 1];
  for (int i = 0; i < length; i++) {
    string[i] = read_byte(data);
  }
  return string;
}

std::uint32_t midiParser::read_value(std::uint8_t **data) {
  std::uint32_t result = read_byte(data);
  std::uint8_t byte = 0;
  if (result & 0b10000000) {
    result &= 0b01111111;
    do {
      byte = read_byte(data);
      result = (result << 7) | (byte & 0b01111111);
    } while (byte & 0b10000000);
  }
  return result;
}

void midiParser::track::add_note(std::uint8_t id, std::uint8_t vel, std::uint8_t chan) {
  std::uint8_t piano_key = id - C1_NOTE + 1;
  std::uint8_t remainder = piano_key % 12;
  std::uint8_t level = (remainder == 0) ? (piano_key / 12) : (piano_key / 12) + 1;
  float frequency = notes_data[remainder].base_freq * pow(2, level - 1);
  std::stringstream ss;
  ss << notes_data[remainder].name << +level;
  std::string note_name = ss.str();
  // std::printf("Note: %s, freq: %.2f Hz\n", note_name.c_str(), frequency);
  struct note _note = {id, vel, chan};
  std::strcpy(_note.name, note_name.c_str());
  _note.frequency = frequency;
  _note.start = this->time_passed;
  _note.duration = 0;
  this->notes.push_back(_note);
}

void midiParser::track::end_note(std::uint8_t id) {
  if (this->notes.size() == 0) return;
  for (auto it = this->notes.rbegin(); it != this->notes.rend(); it++) {
    if (it->id == id) {
      it->duration = this->time_passed - it->start;
      return;
    }
  }

}

midiParser::track midiParser::read_track(std::uint8_t *data, std::uint32_t data_length) {
  std::stringstream ss;
  ss << "Track " << (++this->tracks);
  struct track track_data;
  track_data.name = ss.str();
  track_data.instrument = "Unknown";
  std::uint8_t previous_status = 0;
  std::uint8_t *data_start = data;
  while (data - data_start < data_length) {
    std::uint32_t delta_time = read_value(&data);
    std::uint8_t status = read_byte(&data);
    track_data.time_passed += delta_time;
    if (status < 0x80) {
      // Handling compression
      status = previous_status;
      data--;
    }
    std::uint8_t old_prev_status = previous_status;
    previous_status = status;
    // std::printf("Status: 0x%.2X\n", status);
    if ((status & 0xF0) == note_off) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t note = read_byte(&data);
      std::uint8_t velocity = read_byte(&data);
      // std::cout << "Note off: " << +note << ", vel: " << +velocity << "\n";
      track_data.end_note(note);
    } else if ((status & 0xF0) == note_on) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t note = read_byte(&data);
      std::uint8_t velocity = read_byte(&data);
      if (velocity == 0) {
        // std::cout << "Note off: " << +note << ", vel: " << +velocity << "\n";
        track_data.end_note(note);
      } else {
        // std::cout << "Note on: " << +note << ", vel: " << +velocity << "\n";
        track_data.add_note(note, velocity, channel);
      }
    } else if ((status & 0xF0) == note_after_touch) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t note = read_byte(&data);
      std::uint8_t velocity = read_byte(&data);
    } else if ((status & 0xF0) == note_control_change) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t note = read_byte(&data);
      std::uint8_t velocity = read_byte(&data);
    } else if ((status & 0xF0) == note_program_change) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t program = read_byte(&data);
    } else if ((status & 0xF0) == note_channel_pressure) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t pressure = read_byte(&data);
    } else if ((status & 0xF0) == note_pitch_bend) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t lsb = read_byte(&data);
      std::uint8_t msb = read_byte(&data);
    } else if ((status & 0xF0) == system_exclusive) {
      // std::cout << "System exclusive\n";
      previous_status = 0;
      if (status == 0xF0 || status == 0xF7) {
        char *message = read_string(&data, read_byte(&data));
        // printf("System message %s: '%s'\n", status == 0xF0 ? "begin" : "end", message);
        // std::cout << "System message " << (status == 0xF0 ? "begin '" : "end '") << message << "'\n";
        delete message;
      }
      if (status == 0xFF) {
        std::uint8_t meta_type = read_byte(&data);
        std::uint8_t meta_length = read_byte(&data);
        if (meta_type == meta_sequence) {
          std::cout << "Sequence number: " << read_byte(&data) << " " << read_byte(&data) << "\n";
        } else if (meta_type == meta_text || meta_type == meta_copyright || meta_type == meta_trackname ||
                  meta_type == meta_instrument || meta_type == meta_lyrics || meta_type == meta_marker ||
                  meta_type == meta_cue || meta_type == meta_sequencer) {
          char *message = read_string(&data, meta_length);
          std::cout << "Meta (type: " << +meta_type << "): " << message << "\n";
          if (meta_type == meta_instrument) {
            track_data.instrument = message;
          }
          if (meta_type == meta_trackname) {
            track_data.name = message;
          }
          delete message;
        } else if (meta_type == meta_channel_prefix) {
          std::cout << "Channel prefix: " << read_byte(&data) << "\n";
        } else if (meta_type == meta_eot) {
          // end of track
          break;
        } else if (meta_type == meta_tempo_set) {
          // to do
          if (!track_data.tempo_set) {
            std::cout << "Set tempo\n";
            read_byte(&data);
            read_byte(&data);
            read_byte(&data);
            track_data.tempo_set = true;
          }
        } else if (meta_type == meta_SMPTEOffset) {
          std::printf("SMPTE: %d %d %d %d %d\n", read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data));
        } else if (meta_type == meta_time_signature) {
          std::cout << "Time signature: " << +read_byte(&data) << "/" << +(2 << read_byte(&data)) << "\n";
          std::cout << "Clocks per tick: " << +read_byte(&data) << "\n";
          std::cout << "32 per 24 clocks: " << +read_byte(&data) << "\n";
        } else if (meta_type == meta_key_signature) {
          std::cout << "Key signature: " << +read_byte(&data) << "\n";
          std::cout << "Minor key: " << +read_byte(&data) << "\n";
        } else {
          previous_status = old_prev_status;
          std::cout << "Unknown system event!\n";
        }
      }
    } else {
      // unknown status!
      previous_status = old_prev_status;
      std::cout << "Unknown midi status!\n";
    }
  }
  std::cout << "End of track\n";
  return track_data;
}