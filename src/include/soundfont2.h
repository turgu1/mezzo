// Notice
// ------
//
// This file is part of the Mezzo SoundFont2 Sampling Based Synthesizer:
//
//     https://github.com/turgu1/mezzo
//
// Simplified BSD License
// ----------------------
//
// Copyright (c) 2018, Guy Turcotte
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//

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
