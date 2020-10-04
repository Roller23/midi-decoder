#ifndef __MIDI_
#define __MIDI_

#include <cstdint>
#include <string>
#include <cstdio>
#include <fstream>
#include <vector>

class midiParser {
  private:
    static const int CHUNK_META_LENGTH = 8;
    static const int CHUNK_META_NAME_LENGTH = 4;
    static const int MAX_NOTES_HELD = 10;
    static const int C1_NOTE = 24;
  public:
    struct __attribute__((packed)) chunk {
      union {
        std::uint8_t chunk_meta[CHUNK_META_LENGTH];
        struct {
          std::uint8_t name[CHUNK_META_NAME_LENGTH];
          std::uint32_t size;
        };
      };
      std::uint8_t *data;
    };
    struct header_chunk_data {
      std::uint16_t format;
      std::uint16_t tracks;
      std::uint16_t ticks;
    };
    struct note {
      std::uint8_t id;
      std::uint8_t velocity;
      std::uint8_t channel;
      char name[5];
      std::uint32_t start;
      std::uint32_t duration;
      float frequency;
    };
    struct track {
      std::string name;
      std::string instrument;
      std::vector<struct note> notes;
      void add_note(std::uint8_t id, std::uint8_t vel, std::uint8_t chan);
      void end_note(std::uint8_t id);
      uint32_t time_passed = 0;
    };
    midiParser(std::string midi_filename);
    void read_chunk(chunk *_chunk);
    void free_chunk(chunk *_chunk);
    std::uint8_t read_byte(std::uint8_t **data);
    char *read_string(std::uint8_t **data, std::uint8_t length);
    std::uint32_t read_value(std::uint8_t **data);
    struct track read_track(std::uint8_t *data, std::uint32_t data_length);
    ~midiParser(void);
  private:
    enum system_event {
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
    };
    enum midi_event {
      note_off = 0x80,
      note_on = 0x90,
      note_after_touch = 0xA0,
      note_control_change = 0xB0,
      note_program_change = 0xC0,
      note_channel_pressure = 0xD0,
      note_pitch_bend = 0xE0,
      system_exclusive = 0xF0
    };
    struct note_meta {
      char name[3];
      float base_freq;
    };
    std::ifstream midi_file;
    static const struct note_meta notes_data[12];
    uint32_t tracks = 0;
};

#endif // __MIDI_