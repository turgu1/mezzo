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

#include <iostream>

#include "mezzo.h"

Instrument::Instrument(char * instrumentName, uint16_t bagIndex, uint16_t bagQty)
{
  setNewHandler(outOfMemory);

  name = instrumentName;

  bagIdx   = bagIndex;
  bagCount = bagQty;

  init();

  // logger.DEBUG("Instrument [%s] created.", name.c_str());
}

void Instrument::init()
{
  gens  = NULL;
  mods  = NULL;
  zones = NULL;

  zoneCount = 0;
  globalZonePresent = false;

  globalZone.generators = NULL;
  globalZone.modulators = NULL;
  globalZone.genCount   =    0;
  globalZone.modCount   =    0;

  for (int i = 0; i < 128; i++) keys[i] = KEY_NOT_USED;

  loaded  = false;
}

Instrument::~Instrument()
{
  if (loaded) unload();
}

void Instrument::outOfMemory()
{
  logger.FATAL("Instrument: Unable to allocate memory.");
}

bool Instrument::unload()
{
  if (!loaded) return false;

  if (zones) delete [] zones;
  if (gens)  delete [] gens;
  if (mods)  delete [] mods;

  init();
  return true;
}

bool Instrument::load(sfBag      * bags,
                      sfGenList  * generators,
                      sfModList  * modulators,
                      rangesType & keysToLoad)
{
  int count;

  if (bagCount == 0) return false;

  if (!loaded) {

    // We get one more as an end of list indicator (sampleIndex will be -1)

    zones     = new aZone[bagCount + 1];
    zoneCount = bagCount;

    for (uint16_t zoneIdx = 0; zoneIdx <= zoneCount; zoneIdx++) {
      zones[zoneIdx].keys.byHi       =    0;
      zones[zoneIdx].keys.byLo       =    0;
      zones[zoneIdx].velocities.byLo =    0;
      zones[zoneIdx].velocities.byHi =    0;
      zones[zoneIdx].sampleIndex     =   -1;
      zones[zoneIdx].generators      = NULL;
      zones[zoneIdx].modulators      = NULL;
      zones[zoneIdx].genCount        =    0;
      zones[zoneIdx].modCount        =    0;
    }

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
    // a global one. The last gen in the zone must not be a sampleID Operator

    globalZonePresent = globalZonePresent ||
      (((lastGenIdx - firstGenIdx) > 0) &&
       (generators[bags[bagIdx + 1].wGenNdx - 1].sfGenOper != sfGenOper_sampleID));

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
          (generators[i].sfGenOper != sfGenOper_sampleID)) genCount++;
    }

    int modCount = lastModIdx - firstModIdx;

    if (genCount > 0) gens = new sfGenList[genCount];
    if (modCount > 0) mods = new sfModList[modCount];

    //if ((genCount > 0) || (modCount > 0))
    {
      uint16_t    zoneIdx  = 0; // index into zones
      sfGenList * g  = gens;
      sfGenList * gg = &generators[bags[bagIdx].wGenNdx];
      sfModList * m  = mods;
      sfModList * mm = &modulators[bags[bagIdx].wModNdx];
      sfBag     * b  = &bags[bagIdx];

      int i = 0;  // How many zone entries we have processed in bags

      // ---- globalZone ----

      // global gens

      // There is global gens if the last gen for the zone is not a sampleId operator

      if (globalGenCount > 0) {

        // There is globals gens so get the gens...
        globalZone.generators = g;
        globalZone.genCount   = globalGenCount;

        while (globalGenCount--) *g++ = *gg++;

        assert(globalZonePresent); // Double check...
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
          if (gg[count - 1].sfGenOper == sfGenOper_sampleID) {
            zones[zoneIdx].generators = g;
            while (count--) {
              switch (gg->sfGenOper) {
                case sfGenOper_keyRange:
                  zones[zoneIdx].keys = gg->genAmount.ranges;
                  if (keys[zones[zoneIdx].keys.byLo] == KEY_NOT_USED) {
                    for (int k = zones[zoneIdx].keys.byLo; k <= zones[zoneIdx].keys.byHi; k++) {
                      if (keys[k] != KEY_NOT_USED) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                      keys[k] = zoneIdx;
                    }
                  }
                  break;
                case sfGenOper_velRange:
                  zones[zoneIdx].velocities = gg->genAmount.ranges;
                  break;
                case sfGenOper_sampleID:
                  zones[zoneIdx].sampleIndex = gg->genAmount.wAmount;
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

        if (zones[zoneIdx].sampleIndex != -1) {
          zoneIdx++;       // Goto next zone
        }
        else {
          zoneCount--;
        }
        b++; i++;  // ... and next bag
      }
    }

    loaded = true;
  }

  for (uint16_t zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) {

    if ((zones[zoneIdx].keys.byHi == 0) && (zones[zoneIdx].keys.byLo == 0)) {
      zones[zoneIdx].keys.byHi = 127;
    };

    if ((zones[zoneIdx].velocities.byHi == 0) && (zones[zoneIdx].velocities.byLo == 0)) {
      zones[zoneIdx].velocities.byHi = 127;
    };

    if ((keysToLoad.byLo <= zones[zoneIdx].keys.byHi) && (keysToLoad.byHi >= zones[zoneIdx].keys.byLo)) {
      if (soundFont->loadSample(zones[zoneIdx].sampleIndex)) {
        zones[zoneIdx].synth.setDefaults(soundFont->samples[zones[zoneIdx].sampleIndex]);
        zones[zoneIdx].synth.initGens(globalZone.generators, globalZone.genCount);
        zones[zoneIdx].synth.setGens(zones[zoneIdx].generators, zones[zoneIdx].genCount);
        zones[zoneIdx].synth.completeParams(60);
      }
      else {
        logger.DEBUG("Unable to load sample for zones[%d] sampleIndex %d (zoneCount: %d)", zoneIdx, zones[zoneIdx].sampleIndex, zoneCount);
        showZone(zoneIdx);
      }
    }
  }

  // showZones();

  // logger.DEBUG("Instrument %s (re) loaded.", name.c_str());
  return true;
}

void Instrument::showGenerator(sfGenList & g)
{
  const generatorDescriptor & desc = generatorsDesc[g.sfGenOper];
  std::cerr << desc.name << " [" <<
    g.genAmount.shAmount << "](" << desc.unit << ")";
}

void Instrument::showModInfo(sfModulator & m)
{
  std::cerr << "[I:" << m.index
            << " M:" << m.midiContinuousControllerFlag
            << " D:" << m.direction
            << " P:" << m.polarity
            << " T:" << m.type << "]";
}

void Instrument::showModulator(sfModList & m)
{
  std::cerr << "ModSrc ";
  showModInfo(m.sfModSrcOper);
  std::cerr << " Dest [" << generatorsDesc[m.sfModDestOper].name << "]"
            << " Amount [" << m.modAmount << "]"
            << " ModAmtSrc ";
  showModInfo(m.sfModAmtSrcOper);
  std::cerr << " Transform [" << (m.sfModTransOper == 0 ? "Linear" : "AbsValue") << "]";
}

void Instrument::showZone(uint16_t zoneIdx)
{
  using namespace std;

  cout << "Zone "    <<  zoneIdx                        << ": " <<
    "keys ["         << +zones[zoneIdx].keys.byLo       << "-"  << +zones[zoneIdx].keys.byHi       << "] " <<
    "velocities ["   << +zones[zoneIdx].velocities.byLo << "-"  << +zones[zoneIdx].velocities.byHi << "] " <<
    "sample index [" <<  zones[zoneIdx].sampleIndex     << "] " <<
    "gen count ["    << +zones[zoneIdx].genCount        << "] " <<
    "mod count ["    << +zones[zoneIdx].modCount        << "] " << endl;

  soundFont->samples[zones[zoneIdx].sampleIndex]->showStatus(2);
  zones[zoneIdx].synth.showStatus(2);
  
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

void Instrument::showZones()
{
  using namespace std;

  cerr << endl << "Zone List for Instrument [" << name << "]" << endl << endl;

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
  for (int zoneIdx = 0; zoneIdx < zoneCount; zoneIdx++) showZone(zoneIdx);

  cerr << endl << "[End]" << endl;
}

void Instrument::playNote(uint8_t note,
                          uint8_t velocity,
                          Preset & preset,
                          uint16_t presetZoneIdx)
{
  uint16_t zoneIdx = keys[note];

  bool unblock = poly->getVoiceCount() == 0;
  //bool someNote = false;

  if (zoneIdx == KEY_NOT_USED) return;

  for (; zoneIdx < zoneCount; zoneIdx++) {
    if ((zones[zoneIdx].keys.byLo <= note) &&
        (note <= zones[zoneIdx].keys.byHi) &&
        (zones[zoneIdx].velocities.byLo <= velocity) &&
        (velocity <= zones[zoneIdx].velocities.byHi)) {
      assert(zones[zoneIdx].sampleIndex < (int16_t) soundFont->samples.size());
      assert(soundFont->samples[zones[zoneIdx].sampleIndex] != NULL);
      //someNote = true;
      poly->addVoice(
        soundFont->samples[zones[zoneIdx].sampleIndex],
        note, velocity / 127.0f,
        zones[zoneIdx].synth,
        preset, presetZoneIdx);
      if (unblock) {
        unblock = false;
        poly->UnblockVoiceThreads();
      }
    }
  }

  //if (someNote && unblock) poly->UnblockVoiceThreads();
}

void Instrument::stopNote(uint8_t note)
{
  int zoneIdx = keys[note];
  if (zoneIdx == KEY_NOT_USED) logger.ERROR("keys for note %d not found!", note);
  while ((zoneIdx < zoneCount) &&
         (zones[zoneIdx].sampleIndex != -1) &&
         (zones[zoneIdx].keys.byLo <= note) &&
         (note <= zones[zoneIdx].keys.byHi)) {
    // soundFont->samples[zones[zoneIdx].sampleIndex]->stopNote(note);
    zoneIdx++;
  }
}
