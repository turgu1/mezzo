#ifndef _INSTRUMENT_
#define _INSTRUMENT_

#include <string>

#include "sf2.h"

struct aZone {
  rangesType   keys;
  rangesType   velocities;
  int16_t      sampleIndex;
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

class Instrument
{
private:
  std::string   name;         ///< The name of the instrument
  aGlobalZone   globalZone;   ///< The global zone
  aZone       * zones;
  aZone       * keys[128]; ///< Shortcut to first zone related to a key
  sfGenList   * gens;
  sfModList   * mods;
  int           zoneCount;
  bool          globalZonePresent;

  bool loaded;            ///< True if the instrument content (samples) has been loaded in memory

public:
  Instrument(char      * instrumentName,
             int         bagIdx,
             int         bagCount,
             sfBag     * bags,
             sfGenList * generators,
             sfModList * modulators);
  ~Instrument();

  /// Returns true if the instrument has been loaded in memory
  bool isLoaded() { return loaded; }

  /// Loads / Unloads the instrument information in memory.
  bool load();
  bool unload();

  /// Utils
  void showGenerator(sfGenList & g);
  void showModInfo(sfModulator & m);
  void showModulator(sfModList & m);
  void showZones();

  /// Returns the name of the instrument
  std::string & getName() { return name; }
};

#endif
