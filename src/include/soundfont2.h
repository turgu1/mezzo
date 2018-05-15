#include "copyright.h"

#ifndef _SOUNDFONT2_
#define _SOUNDFONT2_

#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <string>
#include <vector>

/// A Sound Font Version 2 is a self contain audio samples based suite of
/// instruments and presets ready for consomption by a sampler based
/// synthesizer. It is the foundation of the *mezzo* application.
///
/// This class implements all the basic access methods to a soundfont2
/// library.

class SoundFont2  : public NewHandlerSupport<SoundFont2> {

private:

  boost::iostreams::mapped_file_source file;
  const char * data;
  bool loaded;

  chunk     * findChunk    (char const * id, chunkList & src);
  chunkList * findChunkList(char const * name);

  bool retrieveInstrumentList();
  bool retrievePresetList();
  bool retrieveSamples();
  void addPresetToMidiList(Preset *preset);

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

  Preset * currentPreset;
  Preset * firstMidiPreset;

public:

  std::vector<Instrument *> instruments;
  std::vector<Preset *>     presets;
  std::vector<Sample *>     samples;

  /// Open a sound font version 2 file. This will retrieve the list
  /// of instruments and presets present in the sound font.
  SoundFont2(std::string & sf2Filename);
  ~SoundFont2();

  bool loadInstrument(std::string & instrumentName,
                      rangesType  & keys);
  bool loadInstrument(uint16_t instrumentIndex,
                      rangesType  & keys);

  bool loadPreset(std::string & presetName);
  bool loadPreset(uint16_t presetIndex);
  bool loadPreset(Preset * p);
  bool loadMidiPreset(uint8_t bankNbr, uint8_t midiNbr);

  bool loadNextPreset();
  bool loadPreviousPreset();
  bool loadFirstPreset();

  std::vector<uint16_t> showMidiPresetList();

  inline Preset * getCurrentPreset() { return currentPreset; };

  inline bool loadSample(uint16_t sampleIndex) {
    if (sampleIndex < samples.size()) {
      assert(samples[sampleIndex] != NULL);
      return samples[sampleIndex]->load();
    }
    return false;
  };

  inline void playNote(uint8_t note, uint8_t velocity) {
    if (currentPreset) currentPreset->playNote(note, velocity);
  };

  inline void stopNote(uint8_t note) {
    if (currentPreset) currentPreset->stopNote(note);
  }

  inline Instrument * getInstrument(uint16_t index) {
    return index < instruments.size() ? instruments[index] : NULL;
  };
};

#endif
