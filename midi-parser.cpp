#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <byteswap.h>
#include <string>
#include <sstream>

#include "midi-parser.hpp"

midiParser::midiParser(std::string midi_filename) {
  this->midi_file.open(midi_filename, std::ios::binary);
}

midiParser::~midiParser(void) {
  this->midi_file.close();
}

void midiParser::read_chunk(chunk *_chunk) {
  std::memset(_chunk, 0, sizeof(chunk));
  this->midi_file.read((char *)_chunk->chunk_meta, CHUNK_META_LENGTH);
  _chunk->size = __bswap_32(_chunk->size);
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

char *midiParser::read_string(std::uint8_t **data) {
  std::uint8_t length = read_byte(data);
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

void midiParser::add_note(std::uint8_t id, std::uint8_t vel, std::uint8_t chan, std::uint32_t delta) {
  std::uint8_t piano_key = id - C1_NOTE + 1;
  std::uint8_t remainder = piano_key % 12;
  std::uint8_t level = (remainder == 0) ? (piano_key / 12) : (piano_key / 12) + 1;
  float frequency = notes_data[remainder].base_freq * pow(2, level - 1);
  std::stringstream ss;
  ss << notes_data[remainder].name << +level;
  std::string note_name = ss.str();
  std::printf("Note: %s, freq: %.2f Hz\n", note_name.c_str(), frequency);
  struct note _note = {id, vel, chan};
  std::strcpy(_note.name, note_name.c_str());
  _note.frequency = frequency;
  _note.start = this->time_passed;
  _note.duration = 0;
  this->notes.push_back(_note);
}

void midiParser::end_note(std::uint8_t id, std::uint32_t delta) {
  if (this->notes.size() == 0) return;
  for (auto it = this->notes.rbegin(); it != this->notes.rend(); it++) {
    if (it->id == id) {
      it->duration = this->time_passed - it->start;
      return;
    }
  }

}

void midiParser::read_track(std::uint8_t *data, std::uint32_t data_length) {
  std::uint8_t previous_status = 0;
  std::uint8_t *data_start = data;
  while (data - data_start < data_length) {
    std::uint32_t delta_time = read_value(&data);
    std::uint8_t status = read_byte(&data);
    this->time_passed += delta_time;
    if (!(status & 0b10000000)) {
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
      std::cout << "Note off: " << +note << ", vel: " << +velocity << "\n";
      end_note(note, delta_time);
    } else if ((status & 0xF0) == note_on) {
      std::uint8_t channel = status & 0x0F;
      std::uint8_t note = read_byte(&data);
      std::uint8_t velocity = read_byte(&data);
      if (velocity == 0) {
        // 21 is A0
        std::cout << "Note off: " << +note << ", vel: " << +velocity << "\n";
        end_note(note, delta_time);
      } else {
        std::cout << "Note on: " << +note << ", vel: " << +velocity << "\n";
        add_note(note, velocity, channel, delta_time);
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
        char *message = read_string(&data);
        // printf("System message %s: '%s'\n", status == 0xF0 ? "begin" : "end", message);
        std::cout << "System message " << (status == 0xF0 ? "begin '" : "end '") << message << "'\n";
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
          char *message = read_string(&data);
          std::cout << "Meta (type: " << meta_type << "): " << message << "\n";
          delete message;
        } else if (meta_type == meta_channel_prefix) {
          std::cout << "Channel prefix: " << read_byte(&data) << "\n";
        } else if (meta_type == meta_eot) {
          // end of track
          break;
        } else if (meta_type == meta_tempo_set) {
          // to do
          std::cout << "Set tempo\n";
          // read_byte(&data);
          // read_byte(&data);
          // read_byte(&data);
        } else if (meta_type == meta_SMPTEOffset) {
          std::printf("SMPTE: %d %d %d %d %d\n", read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data));
        } else if (meta_type == meta_time_signature) {
          std::cout << "Time signature: " << read_byte(&data) << "/" << (read_byte(&data) << 2) << "\n";
          std::cout << "Clocks per tick: " << read_byte(&data) << "\n";
          std::cout << "32 per 24 clocks: " << read_byte(&data) << "\n";
        } else if (meta_type == meta_key_signature) {
          std::cout << "Key signature: " << read_byte(&data) << "\n";
          std::cout << "Minor key: " << read_byte(&data) << "\n";
        } else {
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
  std::cout << "Notes:\n";
  for (auto &note : this->notes) {
    std::cout << note.name << " s: " << +note.start << " d: " << +note.duration << "\n";
  }
}