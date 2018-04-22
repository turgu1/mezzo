#ifndef _SOUNDFONT2_
#define _SOUNDFONT2_

#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "instrument.h"
#include "preset.h"


/// A Sound Font Version 2 is a self contain audio samples based suite of
/// instruments and presets ready for consomption by a sampler based
/// synthesizer. It is the foundation of the *mezzo* application.
///
/// This class implements all the basic access methods to a soundfont2
/// library. 

class SoundFont2 {

public:
  
  std::vector<Instrument *> instruments;
  std::vector<Preset *>     presets;
  
  /// Open a sound font version 2 file. This will retrieve the list
  /// of instruments and presets present in the sound font.
  SoundFont2(std::string & sf2Filename);
  ~SoundFont2();
  
  bool loadInstrument(std::string & instrumentName);
  bool loadInstrument(int instrumentIndex);
  
  bool loadPreset(std::string & presetName);
  bool loadPreset(int presetIndex);
  
private:
  
  boost::iostreams::mapped_file_source file;
  const char * data;
  bool loaded;
  
  chunk     * findChunk    (char const * id, chunkList & src);
  chunkList * findChunkList(char const * name);
  
  bool retrieveInstrumentList();
  bool retrievePresetList();  
};

#endif
