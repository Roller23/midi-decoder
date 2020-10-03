#include <iostream>
#include <byteswap.h>

#include "midi-parser.hpp"

int main(void) {
  midiParser parser("zelda.mid");
  midiParser::chunk header_chunk;
  parser.read_chunk(&header_chunk);
  midiParser::header_chunk_data header_data = *(midiParser::header_chunk_data *)header_chunk.data;
  header_data.format = __bswap_16(header_data.format);
  header_data.tracks = __bswap_16(header_data.tracks);
  uint8_t timing_type = !!(header_data.ticks & 0x4000); // bit 15
  std::cout << "Format = " << header_data.format << "\n";
  std::cout << "Tracks = " << header_data.tracks << "\n";
  std::cout << "Timing type " << (timing_type ? "timecode" : "metrical timing") << "\n";
  midiParser::chunk *track_chunks = new midiParser::chunk[header_data.tracks];
  for (int i = 0; i < header_data.tracks; i++) {
    std::cout << "Reading track chunk no. " << (i + 1) << std::endl;
    parser.read_chunk(track_chunks + i);
  }
  if (header_data.format == 1) {
    parser.read_track(track_chunks[1].data, track_chunks[1].size);
  }
  parser.free_chunk(&header_chunk);
  parser.close();
  return 0;
}