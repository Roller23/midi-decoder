#include <iostream>

#include "midi-parser.hpp"

int main(void) {
  midiParser parser("dirth.mid");
  midiParser::chunk header_chunk;
  parser.read_chunk(&header_chunk);
  midiParser::header_chunk_data header_data = *(midiParser::header_chunk_data *)header_chunk.data;
  header_data.format = midiParser::bswap(header_data.format);
  header_data.tracks = midiParser::bswap(header_data.tracks);
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
    for (int i = 0; i < header_data.tracks; i++) {
      midiParser::track _track = parser.read_track(track_chunks[i].data, track_chunks[i].size);
      std::cout << "Track name: " << _track.name << "\n";
      std::cout << "Instrument: " << _track.instrument << "\n";
      std::cout << "Notes:\n";
      for (auto &note : _track.notes) {
        // std::cout << note.name << ", start: " << note.start << ", duration: " << note.duration << "\n";
      }
      std::cout << std::endl;
    }
  }
  parser.free_chunk(&header_chunk);
  return 0;
}