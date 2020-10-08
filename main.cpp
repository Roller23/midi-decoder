#include <iostream>

#include "midi-parser.hpp"

int main(void) {
  midiParser parser;
  parser.errors = true;
  parser.verbose = true;
  midiParser::midi_data data = parser.parse_file("zelda.mid");
  std::cout << "Format: " << +data.format << "\n";
  std::cout << "Tracks: " << +data.tracks_count << "\n";
  std::cout << "Timing: " << +data.timing << "\n";
  for (auto &track : data.tracks) {
    std::cout << "Track name: " << track.name << "\n";
    std::cout << "Instrument: " << track.instrument << "\n";
    std::cout << "Notes:\n";
    for (auto &note : track.notes) {
      std::printf("%s %.2f Hz\n", note.name, note.frequency);
    }
  }
  return 0;
}