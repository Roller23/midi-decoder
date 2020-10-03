#ifndef __MIDI_
#define __MIDI_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define CHUNK_META_LENGTH 8
#define CHUNK_META_NAME_LENGTH 4
#define MAX_NOTES_HELD 10
#define C1_NOTE 24 // starting point

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

typedef struct {
  char name[3];
  float base_freq;
} note_meta;

void read_chunk(FILE *midi_file, chunk *_chunk);
void free_chunk(chunk *_chunk);
uint8_t read_byte(uint8_t **data);
char *read_string(uint8_t **data);
uint32_t read_value(uint8_t **data);
void get_note_name(uint8_t id);
void read_track(uint8_t *data, uint32_t data_length);

#endif // __MIDI_