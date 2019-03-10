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

#include "mezzo.h"

Preset::Preset(char * presetName,
               uint16_t midi,
               uint16_t bank,
               uint16_t bagIndex,
               uint16_t bagQty)
{
  setNewHandler(outOfMemory);

  name           = presetName;
  midiNbr        = midi;
  bankNbr        = bank;

  bagIdx         = bagIndex;
  bagCount       = bagQty;

  nextMidiPreset = NULL;

  init();

  // logger.DEBUG("Preset [%s] created.", name.c_str());
}

void Preset::init()
{
  for (int i = 0; i < 128; i++) keys[i] = KEY_NOT_USED;

  gens                  = NULL;
  mods                  = NULL;
  zones                 = NULL;
  zoneCount             =    0;

  globalZone.generators = NULL;
  globalZone.modulators = NULL;
  globalZone.genCount   =    0;
  globalZone.modCount   =    0;


  keyShortCutPresent    = false;
  velocitiesPresent     = false;

  loaded                = false;
}

Preset::~Preset()
{
  if (loaded) unload();

  for (unsigned i = 0; i < instruments.size(); i++) {
    delete instruments[i];
  }
}

void Preset::outOfMemory()
{
  logger.FATAL("Preset: Unable to allocate memory.");
}

bool Preset::unload()
{
  if (!loaded) return false;
  assert(soundFont != NULL);

  for (int zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) {
    if (zones[zoneIdx].instrumentIndex >= 0) {
      assert(zones[zoneIdx].instrumentIndex < (int) soundFont->instruments.size());
      assert(soundFont->instruments[zones[zoneIdx].instrumentIndex] != NULL);
      soundFont->instruments[zones[zoneIdx].instrumentIndex]->unload();
    }
  }

  if (zones) delete [] zones;
  if (gens)  delete [] gens;
  if (mods)  delete [] mods;

  init();
  return true;
}

bool Preset::load(sfBag     * bags,
                  sfGenList * generators,
                  sfModList * modulators)
{
  int count;

  if (loaded) return true;

  if (bagCount == 0) {
    logger.DEBUG("Bag Count is 0 for preset %s", name.c_str());
    return false;
  }

  // We get one more as an end of list indicator (instrumentIndex will be -1)

  zones     = new aZone[bagCount + 1];
  zoneCount = bagCount;

  for (int zoneIdx = 0; zoneIdx <= zoneCount; zoneIdx++) {
    zones[zoneIdx].keys.byHi       =    0;
    zones[zoneIdx].keys.byLo       =    0;
    zones[zoneIdx].velocities.byLo =    0;
    zones[zoneIdx].velocities.byHi =    0;
    zones[zoneIdx].instrumentIndex =   -1;
    zones[zoneIdx].generators      = NULL;
    zones[zoneIdx].modulators      = NULL;
    zones[zoneIdx].genCount        =    0;
    zones[zoneIdx].modCount        =    0;
  }

  zones[zoneCount].instrumentIndex = -999;

  int firstGenIdx = bags[bagIdx].wGenNdx;
  int lastGenIdx  = bags[bagIdx + bagCount].wGenNdx;
  int firstModIdx = bags[bagIdx].wModNdx;
  int lastModIdx  = bags[bagIdx + bagCount].wModNdx;

  // If there is no generators in the bags but only modulators
  // that means there is only a global zone.

  globalZonePresent =
    ((lastGenIdx - firstGenIdx) == 0) &&
    ((lastModIdx - firstModIdx) > 0);

  // If there is some generators, check if the first zone is
  // a global one.

  globalZonePresent = globalZonePresent ||
    (((lastGenIdx - firstGenIdx) > 0) &&
     (generators[bags[bagIdx + 1].wGenNdx - 1].sfGenOper != sfGenOper_instrumentID));

  uint8_t globalGenCount;
  if (globalZonePresent) {
    zoneCount--;
    globalGenCount = bags[bagIdx + 1].wGenNdx - bags[bagIdx].wGenNdx;
  }
  else {
    globalGenCount = 0;
  }

  // Count how many generators need to be allocated, considering
  // that some generator values will be retrieved as normal
  // instrument parameters.

  int genCount = globalGenCount;

  for (int i = firstGenIdx + globalGenCount; i < lastGenIdx; i++) {
    if ((generators[i].sfGenOper != sfGenOper_keyRange) &&
        (generators[i].sfGenOper != sfGenOper_velRange) &&
        (generators[i].sfGenOper != sfGenOper_instrumentID)) genCount++;
  }

  int modCount = lastModIdx - firstModIdx;

  if (genCount > 0) gens = new sfGenList[genCount];
  if (modCount > 0) mods = new sfModList[modCount];

  //if ((genCount > 0) || (modCount > 0))
  {
    uint16_t    zoneIdx = 0;   // Index into zones
    sfGenList *       g = gens;
    sfGenList *      gg = &generators[bags[bagIdx].wGenNdx];
    sfModList *       m = mods;
    sfModList *      mm = &modulators[bags[bagIdx].wModNdx];
    sfBag     *       b = &bags[bagIdx];

    int i = 0;  // How many zone entries we have processed in bags

    // ---- globalZone ----

    // global gens

    if (globalGenCount > 0) {

      // There is globals gens so get the gens...
      globalZone.generators = g;
      globalZone.genCount   = globalGenCount;

      while (globalGenCount--) *g++ = *gg++;

      assert(globalZonePresent);
    }

    // global mods

    if (globalZonePresent) {

      count = b[1].wModNdx - b[0].wModNdx;
      if (count > 0) {
        globalZone.modulators = m;
        globalZone.modCount   = count;

        while (count--) *m++ = *mm++;
      }
      b++; i++;
    }

    while (i < bagCount) {
      // Loop for each zone

      // ---- generators ----

      // Get the number of generators in the zone
      count = b[1].wGenNdx - b[0].wGenNdx;

      if (count > 0) {
        if (gg[count - 1].sfGenOper == sfGenOper_instrumentID) {
          zones[zoneIdx].generators = g;
          while (count--) {
            switch (gg->sfGenOper) {
              case sfGenOper_keyRange:
                zones[zoneIdx].keys = gg->genAmount.ranges;
                //if (keys[zones[zoneIdx].keys.byLo] == KEY_NOT_USED) {
                  for (int k = zones[zoneIdx].keys.byLo; k <= zones[zoneIdx].keys.byHi; k++) {
                    if (keys[k] == KEY_NOT_USED) {
                      keys[k] = zoneIdx;
                      keyShortCutPresent = true;
                    }
                  }
                //}
                break;
              case sfGenOper_velRange:
                zones[zoneIdx].velocities = gg->genAmount.ranges;
                velocitiesPresent = true;
                break;
              case sfGenOper_instrumentID:
                zones[zoneIdx].instrumentIndex = gg->genAmount.wAmount;
                uint16_t k;
                for (k = 0; k < instruments.size(); k++) {
                  if (instruments[k]->index == zones[zoneIdx].instrumentIndex) break;
                }
                if (k >= instruments.size()) {
                  presetInstrument * pi = new presetInstrument;
                  assert(pi != NULL);
                  pi->index = zones[zoneIdx].instrumentIndex;
                  Instrument * inst = soundFont->getInstrument(pi->index);
                  assert(inst != NULL);
                  pi->name = inst->getName();
                  instruments.push_back(pi);
                }
                break;
              default:
                *g++ = *gg;
                zones[zoneIdx].genCount++;
                break;
            }
            gg++;
          }
          if (zones[zoneIdx].genCount == 0) zones[zoneIdx].generators = NULL;
        }
        else {
          // The generators list is not valid.
          while (count--) gg++;
        }
      }

      // ---- modulators ----

      count = b[1].wModNdx - b[0].wModNdx;

      if (count > 0) {
        zones[zoneIdx].modulators = m;
        zones[zoneIdx].modCount   = count;
        while (count--) *m++ = *mm++;
      }

      if (zones[zoneIdx].instrumentIndex != -1) {
        zoneIdx++;       // Goto next zone
      }
      else {
        zoneCount--;
      }
      b++; i++;  // ... and next bag
    }
  }

  // Load instruments

  for (uint16_t zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) {
    assert(soundFont != NULL);
    if ((zones[zoneIdx].keys.byHi == 0) && (zones[zoneIdx].keys.byLo == 0)) {
      zones[zoneIdx].keys.byHi = 127;
      for (int keyIdx = 0; keyIdx < 128; keyIdx++) {
        if (keys[keyIdx] == KEY_NOT_USED) keys[keyIdx] = zoneIdx;
      }
    };

    if ((zones[zoneIdx].velocities.byHi == 0) && (zones[zoneIdx].velocities.byLo == 0)) {
      zones[zoneIdx].velocities.byHi = 127;
    };

    soundFont->loadInstrument(zones[zoneIdx].instrumentIndex,
                              zones[zoneIdx].keys);
  }

  // showZones();

  // logger.DEBUG("Preset %s loaded.", name.c_str());

  loaded = true;
  return true;
}

void Preset::showGenerator(sfGenList & g)
{
  const generatorDescriptor & desc = generatorsDesc[g.sfGenOper];
  std::cerr << desc.name << " [" <<
    g.genAmount.shAmount << "](" << desc.unit << ")";
}

void Preset::showModInfo(sfModulator & m)
{
  std::cerr << "[I:" << m.index
            << " M:" << m.midiContinuousControllerFlag
            << " D:" << m.direction
            << " P:" << m.polarity
            << " T:" << m.type << "]";
}

void Preset::showModulator(sfModList & m)
{
  using namespace std;

  cerr << "ModSrc ";
  showModInfo(m.sfModSrcOper);
  cerr << " Dest ["   << generatorsDesc[m.sfModDestOper].name << "]"
       << " Amount [" << m.modAmount << "]"
       << " ModAmtSrc ";
  showModInfo(m.sfModAmtSrcOper);
  cerr << " Transform [" << (m.sfModTransOper == 0 ? "Linear" : "AbsValue") << "]";
}

void Preset::showZone(uint16_t zoneIdx)
{
  using namespace std;

  cerr << "Zone " << zoneIdx << ": " <<
    "keys ["             << +zones[zoneIdx].keys.byLo << "-" << +zones[zoneIdx].keys.byHi << "] " <<
    "velocities ["       << +zones[zoneIdx].velocities.byLo << "-" << +zones[zoneIdx].velocities.byHi << "] " <<
    "instrument index [" <<  zones[zoneIdx].instrumentIndex << "] " <<
    "gen count ["        << +zones[zoneIdx].genCount << "] " <<
    "mod count ["        << +zones[zoneIdx].modCount << "] " << endl;
  if (zones[zoneIdx].genCount > 0) {
    cerr << "  Generators:" << endl;
    for (int j = 0; j < zones[zoneIdx].genCount; j++) {
      cerr << "    " << j << ": ";
      showGenerator(zones[zoneIdx].generators[j]);
      cerr << endl;
    }
  }
  if (zones[zoneIdx].modCount > 0) {
    cerr << "  Modulators:" << endl;
    for (int j = 0; j < zones[zoneIdx].modCount; j++) {
      cerr << "    " << j << ": ";
      showModulator(zones[zoneIdx].modulators[j]);
      cerr << endl;
    }
  }
}

void Preset::showZones()
{
  using namespace std;

  cerr << endl << "Zone List for Preset [" << name << "]" << endl << endl;

  if ((globalZone.genCount > 0) || (globalZone.modCount > 0)) {
    cerr << "Globals :" << endl;
    if (globalZone.genCount > 0) {
      cerr << "  Generators:" << endl;
      for (int j = 0; j < globalZone.genCount; j++) {
        cerr << "    " << j << ": ";
        showGenerator(globalZone.generators[j]);
        cerr << endl;
      }
    }
    if (globalZone.modCount > 0) {
      cerr << "  Modulators:" << endl;
      for (int j = 0; j < globalZone.modCount; j++) {
        cerr << "    " << j << ": ";
        showModulator(globalZone.modulators[j]);
        cerr << endl;
      }
    }
  }
  for (uint16_t zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) showZone(zoneIdx);

  cerr << endl << "[End]" << endl;
}

void Preset::playNote(uint8_t note, uint8_t velocity)
{
  //std::cout << zoneCount << std::endl;

  for (uint16_t zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) {
    if ((zones[zoneIdx].keys.byLo <= note) &&
        (note <= zones[zoneIdx].keys.byHi) &&
        (zones[zoneIdx].velocities.byLo <= velocity) &&
        (velocity <= zones[zoneIdx].velocities.byHi)) {
      soundFont->instruments[zones[zoneIdx].instrumentIndex]->playNote(
        note, velocity, *this, zoneIdx);
    }
    else {
      //std::cout << "No note played: " << +note << "[" << +velocity << "]" << std::endl;
    }
  }
}

void Preset::stopNote(uint8_t note) {
  uint16_t zoneIdx = keys[note];
  while ((zoneIdx < zoneCount) &&
         (zones[zoneIdx].keys.byLo <= note) &&
         (note <= zones[zoneIdx].keys.byHi)) {
    soundFont->instruments[zones[zoneIdx].instrumentIndex]->stopNote(note);
    zoneIdx++;
  }
}
