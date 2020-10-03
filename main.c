#include <stdio.h>
#include <byteswap.h>
#include <stdlib.h>

#include "midi.h"

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