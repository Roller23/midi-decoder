#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#include <stdlib.h>
#include <stdbool.h>

#define CHUNK_META_LENGTH 8
#define CHUNK_META_NAME_LENGTH 4
#define MAX_NOTES_HELD 10

typedef struct __attribute__((packed)) {
  union {
    uint8_t chunk_meta[CHUNK_META_LENGTH];
    struct {
      uint8_t name[CHUNK_META_NAME_LENGTH];
      uint32_t size;
    };
  };
  uint8_t *data;
} chunk;

typedef enum {
  note_on = 0x80,
  note_off = 0x90,
  note_after_touch = 0xA0,
  note_control_change = 0xB0,
  note_program_change = 0xC0,
  note_channel_pressure = 0xD0,
  note_pitch_bend = 0xE0,
  system_exclusive = 0xF0
} midi_event;

typedef enum {
  meta_sequence = 0x00,
  meta_text = 0x01,
  meta_copyright = 0x02,
  meta_trackname = 0x03,
  meta_instrument = 0x04,
  meta_lyrics = 0x05,
  meta_marker = 0x06,
  meta_cue = 0x07,
  meta_channel_prefix = 0x20,
  meta_eot = 0x2F,
  meta_tempo_set = 0x51,
  meta_SMPTEOffset = 0x54,
  meta_time_signature = 0x58,
  meta_key_signature = 0x59,
  meta_sequencer = 0x7F,
} system_event;

typedef struct {
  uint16_t format;
  uint16_t tracks;
  uint16_t ticks;
} header_chunk_data;

typedef struct {
  bool occupied;
  uint8_t id;
  uint8_t velocity;
  uint8_t channel;
  const char name[5];
} note;

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

void read_track(uint8_t *data, uint32_t data_length) {
  uint8_t previous_status = 0;
  uint8_t *data_start = data;
  while (true) {
    if (data - data_start >= data_length) {
      break;
    }
    uint32_t delta_time = read_value(&data);
    uint8_t status = read_byte(&data);
    if (!(status & 0b10000000)) {
      // this file is compressed
      // printf("Note chain\n");
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
      printf("Note off: %d, vel %d\n", note, velocity);

    } else if ((status & 0xF0) == note_on) {
      uint8_t channel = status & 0x0F;
      uint8_t note = read_byte(&data);
      uint8_t velocity = read_byte(&data);
      if (velocity == 0) {
        // 21 is A0
        printf("Note off: %d, vel: %d\n", note, velocity);
      } else {
        printf("Note on: %d, vel: %d\n", note, velocity);
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

int main(void) {
  FILE *zelda = fopen("zelda.mid", "rb");
  if (zelda == NULL) {
    return 1;
  }
  chunk header_chunk;
  read_chunk(zelda, &header_chunk);
  header_chunk_data header_data = *(header_chunk_data *)header_chunk.data;
  header_data.format = __bswap_16(header_data.format);
  header_data.tracks = __bswap_16(header_data.tracks);
  uint8_t timing_type = !!(header_data.ticks & 0x4000); // bit 15
  printf("Format = %d\n", header_data.format);
  printf("Tracks = %d\n", header_data.tracks);
  printf("Timing type = %s\n", timing_type ? "timecode" : "metrical timing");
  chunk *track_chunks = calloc(header_data.tracks, sizeof(chunk));
  for (int i = 0; i < header_data.tracks; i++) {
    printf("Reading track chunk no. %d\n", i + 1);
    read_chunk(zelda, track_chunks + i);
  }
  if (header_data.format == 1) {
    read_track(track_chunks[1].data, track_chunks[1].size);
  }
  fclose(zelda);
  free_chunk(&header_chunk);
  return 0;
}