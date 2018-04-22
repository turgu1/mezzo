#ifndef _PRESET_
#define _PRESET_

#include <string>

#include "sf2.h"

class Preset
{
private:
  std::string name;      ///< The name of the preset
  uint16_t    midiNbr;   ///< The midi number associated with this preset
  uint16_t    bankNbr;   ///< The midi bank number
  bool        loaded;    ///< True if the preset content has been loaded in memory
  
public:
  Preset(char * presetName, uint16_t midi, uint16_t bank);
  ~Preset();
  
  /// Returns true if the preset has been loaded in memory
  bool is_loaded() { return loaded; }
  
  /// Loads / Unloads the preset information in memory. That will include the
  /// loading of associated instruments.
  bool load();
  bool unload();
  
  /// Returns the name of the preset
  std::string & get_name() { return name; }
};

#endif