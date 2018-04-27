#include "copyright.h"

#ifndef _INSTRUMENT_
#define _INSTRUMENT_

#include <string>

#include "sf2.h"

class Instrument : public NewHandlerSupport<Instrument> {

private:
  struct aZone {
    rangesType   keys;
    rangesType   velocities;
    int16_t      sampleIndex;
    int16_t      pan;
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

  uint16_t      bagIdx, bagCount;

  std::string   name;         ///< The name of the instrument
  aGlobalZone   globalZone;   ///< The global zone
  aZone       * zones;
  aZone       * keys[128];    ///< Shortcuts to the first zone related to a key
  sfGenList   * gens;
  sfModList   * mods;
  int           zoneCount;
  bool          globalZonePresent;

  bool loaded;            ///< True if the instrument content (samples) has been loaded in memory

  void init();

  /// Utils
  void showGenerator(sfGenList & g);
  void showModInfo(sfModulator & m);
  void showModulator(sfModList & m);
  void showZones();

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Instrument(char * instrumentName, uint16_t bagIndex, uint16_t bagQty);
  ~Instrument();

  /// Returns true if the instrument has been loaded in memory
  bool isLoaded() { return loaded; }

  /// Loads / Unloads the instrument information in memory.
  bool load(sfBag      * bags,
            sfGenList  * generators,
            sfModList  * modulators,
            rangesType & keysToLoad);
  bool unload();

  void playNote(uint8_t note, uint8_t velocity);
  void stopNote(uint8_t note);

  /// Returns the name of the instrument
  std::string & getName() { return name; }
};

#endif
