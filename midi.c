#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <stdbool.h>

#include "midi.h"

const note_meta notes_data[] = {
  {"B", 61.74}, {"C", 32.70}, {"C#", 34.65}, {"D", 36.71},
  {"D#", 38.89},{"E", 41.20}, {"F", 43.65}, {"F#", 46.25},
  {"G", 49.00}, {"G#", 51.91}, {"A", 55.00}, {"A#", 58.27}
};

void read_chunk(FILE *midi_file, chunk *_chunk) {
  memset(_chunk, 0, sizeof(chunk));
  fread(_chunk->chunk_meta, CHUNK_META_LENGTH, 1, midi_file);
  _chunk->size = __bswap_32(_chunk->size);
  _chunk->data = calloc(1, _chunk->size);
  fread(_chunk->data, _chunk->size, 1, midi_file);
  printf("Chunk name: '%.4s'\n", _chunk->name);
  printf("Chunk size: %d\n", _chunk->size);
}

void free_chunk(chunk *_chunk) {
  free(_chunk->data);
}

uint8_t read_byte(uint8_t **data) {
  uint8_t byte = **data;
  (*data)++;
  return byte;
}

char *read_string(uint8_t **data) {
  uint8_t length = read_byte(data);
  char *string = calloc(length + 1, sizeof(char));
  for (int i = 0; i < length; i++) {
    string[i] = read_byte(data);
  }
  return string;
}

uint32_t read_value(uint8_t **data) {
  uint32_t result = 0;
  do {
    uint8_t byte = read_byte(data);
    uint8_t bit = (byte & 0b10000000);
    result += (byte & 0b01111111) << 1;
    if (bit == 0) {
      break;
    }
  } while (true);
  return result;
}

void get_note_name(uint8_t id) {
  uint8_t piano_key = id - C1_NOTE + 1;
  uint8_t remainder = piano_key % 12;
  uint8_t level = (remainder == 0) ? (piano_key / 12) : (piano_key / 12) + 1;
  float frequency = notes_data[remainder].base_freq * pow(2, level - 1);
  char note_str[5];
  memset(note_str, 0, 5);
  sprintf(note_str, "%s%d", notes_data[remainder].name, level);
  printf("Note: %s, freq: %.2f Hz\n", note_str, frequency);
}

void read_track(uint8_t *data, uint32_t data_length) {
  uint8_t previous_status = 0;
  uint8_t *data_start = data;
  while (data - data_start < data_length) {
    uint32_t delta_time = read_value(&data);
    uint8_t status = read_byte(&data);
    if (!(status & 0b10000000)) {
      // Handling compression
      status = previous_status;
      data--;
    }
    uint8_t old_prev_status = previous_status;
    previous_status = status;
    // printf("Status: 0x%.2X\n", status);
    if ((status & 0xF0) == note_off) {
      uint8_t channel = status & 0x0F;
      uint8_t note = read_byte(&data);
      uint8_t velocity = read_byte(&data);
      // printf("Note off: %d, vel %d\n", note, velocity);
    } else if ((status & 0xF0) == note_on) {
      uint8_t channel = status & 0x0F;
      uint8_t note = read_byte(&data);
      uint8_t velocity = read_byte(&data);
      if (velocity == 0) {
        // 21 is A0
        // printf("Note off: %d, vel: %d\n", note, velocity);
      } else {
        printf("Note on: %d, vel: %d\n", note, velocity);
        get_note_name(note);
      }
    } else if ((status & 0xF0) == note_after_touch) {
      uint8_t channel = status & 0x0F;
      uint8_t note = read_byte(&data);
      uint8_t velocity = read_byte(&data);
    } else if ((status & 0xF0) == note_control_change) {
      uint8_t channel = status & 0x0F;
      uint8_t note = read_byte(&data);
      uint8_t velocity = read_byte(&data);
    } else if ((status & 0xF0) == note_program_change) {
      uint8_t channel = status & 0x0F;
      uint8_t program = read_byte(&data);
    } else if ((status & 0xF0) == note_channel_pressure) {
      uint8_t channel = status & 0x0F;
      uint8_t pressure = read_byte(&data);
    } else if ((status & 0xF0) == note_pitch_bend) {
      uint8_t channel = status & 0x0F;
      uint8_t lsb = read_byte(&data);
      uint8_t msb = read_byte(&data);
    } else if ((status & 0xF0) == system_exclusive) {
      // printf("System exclusive\n");
      previous_status = 0;
      if (status == 0xF0 || status == 0xF7) {
        char *message = read_string(&data);
        // printf("System message %s: '%s'\n", status == 0xF0 ? "begin" : "end", message);
        free(message);
      }
      if (status == 0xFF) {
        uint8_t meta_type = read_byte(&data);
        uint8_t meta_length = read_byte(&data);
        if (meta_type == meta_sequence) {
          printf("Sequence number: %d %d\n", read_byte(&data), read_byte(&data));
        } else if (meta_type == meta_text || meta_type == meta_copyright || meta_type == meta_trackname ||
                  meta_type == meta_instrument || meta_type == meta_lyrics || meta_type == meta_marker ||
                  meta_type == meta_cue || meta_type == meta_sequencer) {
          char *message = read_string(&data);
          printf("Meta (type: %d): %s\n", meta_type, message);
          free(message);
        } else if (meta_type == meta_channel_prefix) {
          printf("Channel prefix: %d\n", read_byte(&data));
        } else if (meta_type == meta_eot) {
          break;
        } else if (meta_type == meta_tempo_set) {
          // to do
          printf("Set tempo\n");
          // read_byte(&data);
          // read_byte(&data);
          // read_byte(&data);
        } else if (meta_type == meta_SMPTEOffset) {
          printf("SMPTE: %d %d %d %d %d\n", read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data), read_byte(&data));
        } else if (meta_type == meta_time_signature) {
          printf("Time signature: %d/%d\n", read_byte(&data), read_byte(&data) << 2);
          printf("Clocks per ticks: %d\n", read_byte(&data));
          printf("32 per 24 clocks: %d\n", read_byte(&data));
        } else if (meta_type == meta_key_signature) {
          printf("Key signature: %d\n", read_byte(&data));
          printf("Minor key: %d\n", read_byte(&data));
        } else {
          // printf("Unknown system event!\n");
        }
      }
    } else {
      // unknown status!
      previous_status = old_prev_status;
      // printf("Unknown midi status!\n");
    }
  }
}