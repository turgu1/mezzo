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

#ifndef _PRESET_
#define _PRESET_

#include <string>

struct presetInstrument {
  std::string name;
  int16_t index;
};

class Preset : public NewHandlerSupport<Preset> {

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

  std::string   name;            ///< The name of the preset

  uint16_t      midiNbr;         ///< The midi number associated with this preset
  uint16_t      bankNbr;         ///< The midi bank number

  Preset      * nextMidiPreset;  ///< Midi/Bank number sort ptr
  uint16_t      bagIdx, bagCount;

  aGlobalZone   globalZone;      ///< The global zone
  aZone       * zones;
  uint16_t      keys[128];       ///< Shortcuts to the first zone related to a key
  sfGenList   * gens;
  sfModList   * mods;
  int           zoneCount;
  bool          globalZonePresent;
  bool          keyShortCutPresent;
  bool          velocitiesPresent;

  bool          loaded;          ///< True if the preset content has been loaded in memory

  std::vector<presetInstrument *> instruments;

  void init();

  /// Utils
  void showGenerator(sfGenList & g);
  void showModInfo(sfModulator & m);
  void showModulator(sfModList & m);

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Preset(char * presetName,
         uint16_t midi,
         uint16_t bank,
         uint16_t bagIndex,
         uint16_t bagQty);
  ~Preset();

  /// Returns true if the preset has been loaded in memory
  bool is_loaded() { return loaded; };

  /// Loads / Unloads the preset information in memory. That will include the
  /// loading / unloading of associated instruments and samples.
  bool load(sfBag     * bags,
            sfGenList * generators,
            sfModList * modulators);
  bool unload();
  void showZone(uint16_t zIdx);
  void showZones();

  void playNote(uint8_t note, uint8_t velocity);
  void stopNote(uint8_t note);

  std::string &         getName() { return name;           }
  uint16_t           getMidiNbr() { return midiNbr;        }
  uint16_t           getBankNbr() { return bankNbr;        }
  Preset *    getNextMidiPreset() { return nextMidiPreset; }
  
  sfGenList *     getGlobalGens() { return globalZone.generators; }
  uint8_t     getGlobalGenCount() { return globalZone.genCount;   }
  
  sfGenList *     getZoneGens(uint16_t idx) { return zones[idx].generators; }
  uint8_t     getZoneGenCount(uint16_t idx) { return zones[idx].genCount;   }

  std::vector<presetInstrument *> & getInstrumentsList() { return instruments; };  

  void     setNextMidiPreset(Preset * p) { nextMidiPreset = p; };
};

#endif
