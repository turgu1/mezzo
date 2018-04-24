#ifndef _PRESET_
#define _PRESET_

#include <string>

#include "sf2.h"

class Preset
{
private:
  struct aZone {
    rangesType   keys;
    rangesType   velocities;
    int16_t      instrumentIndex;
    sfGenList  * generators;
    sfModList  * modulators;
    uint8_t      genCount;
    uint8_t      modCount;
  };

  struct aGlobalZone {
    sfGenList  * generators;
    sfModList  * modulators;
    uint8_t      genCount;
    uint8_t      modCount;
  };

  std::string   name;       ///< The name of the preset
  
  uint16_t      midiNbr;    ///< The midi number associated with this preset
  uint16_t      bankNbr;    ///< The midi bank number

  uint16_t      bagIdx, bagCount;
    
  aGlobalZone   globalZone; ///< The global zone
  aZone       * zones;
  aZone       * keys[128];  ///< Shortcuts to the first zone related to a key
  sfGenList   * gens;
  sfModList   * mods;
  int           zoneCount;
  bool          globalZonePresent;

  bool          loaded;     ///< True if the preset content has been loaded in memory
  
  void init();

  /// Utils
  void showGenerator(sfGenList & g);
  void showModInfo(sfModulator & m);
  void showModulator(sfModList & m);
  void showZones();

public:
  Preset(char * presetName, 
         uint16_t midi, 
         uint16_t bank, 
         uint16_t bagIndex, 
         uint16_t bagQty);
  ~Preset();
  
  /// Returns true if the preset has been loaded in memory
  bool is_loaded() { return loaded; }
  
  /// Loads / Unloads the preset information in memory. That will include the
  /// loading of associated instruments.
  bool load(sfBag     * bags,
            sfGenList * generators,
            sfModList * modulators);
  bool unload();
  
  /// Returns the name of the preset
  std::string & getName() { return name; }
};

#endif