#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <byteswap.h>
#include <stdlib.h>

#define CHUNK_META_LENGTH 8
#define CHUNK_META_NAME_LENGTH 4

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

typedef struct {
  uint16_t format;
  uint16_t tracks;
  uint16_t ticks;
} header_chunk_data;

void read_chunk(FILE *midi_file, chunk *chunk) {
  memset(chunk, 0, sizeof(chunk));
  fread(chunk->chunk_meta, CHUNK_META_LENGTH, 1, midi_file);
  chunk->size = __bswap_32(chunk->size);
  chunk->data = calloc(1, chunk->size);
  fread(chunk->data, chunk->size, 1, midi_file);
}

int main(void) {
  FILE *zelda = fopen("zelda.mid", "rb");
  if (zelda == NULL) {
    return 1;
  }
  chunk header_chunk;
  read_chunk(zelda, &header_chunk);
  printf("First header name: '%4s'\n", header_chunk.name);
  printf("First header size: %d\n", header_chunk.size);
  header_chunk_data header_data = *(header_chunk_data *)header_chunk.data;
  uint8_t timing_type = !!(header_data.ticks & 0x4000); // bit 15
  printf("Format = %d\n", (int)__bswap_16(header_data.format));
  printf("Tracks = %d\n", (int)__bswap_16(header_data.tracks));
  printf("Ticks = %d\n", (int)__bswap_16(header_data.ticks));
  printf("Timing type = %s\n", timing_type ? "timecode" : "metrical timing");
  fclose(zelda);
  return 0;
}