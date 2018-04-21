#ifndef _SF2_
#define _SF2_

#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <string>
#include <forward_list>

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
  
  /// Open a sound font version 2 file. This will retrieve the list
  /// of instruments and presets present in the sound font.
  SoundFont2(std::string & sf2_filename);
  ~SoundFont2();
  
  bool load_instrument(std::string & instrument_name);
  bool load_instrument(int instrument_index);
  
  bool load_preset(std::string & preset_name);
  bool load_preset(int preset_index);
  
private:
  
  boost::iostreams::mapped_file_source file;
  const char * data;
  bool loaded;
  
  std::forward_list<Instrument> instruments;
  std::forward_list<Preset>     presets;
  
  bool retrieve_instrument_list();
  bool retrieve_preset_list();  
};

#endif
