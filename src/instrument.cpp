#include "copyright.h"

#include <iostream>

#include "mezzo.h"

Instrument::Instrument(char * instrumentName, uint16_t bagIndex, uint16_t bagQty)
{
  setNewHandler(outOfMemory);

  name = instrumentName;

  bagIdx   = bagIndex;
  bagCount = bagQty;

  init();

  logger.DEBUG("Instrument [%s] created.", name.c_str());
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
                      rangesType & keysToLoad,
                      Preset     & preset,
                      uint16_t     presetZoneIdx)
{
  int i, count;

  if (bagCount == 0) return false;

  if (!loaded) {

    // We get one more as an end of list indicator (sampleIndex will be -1)

    zones     = new aZone[bagCount + 1];
    zoneCount = bagCount;

    for (i = 0; i <= zoneCount; i++) {
      zones[i].keys.byHi       =    0;
      zones[i].keys.byLo       =    0;
      zones[i].velocities.byLo =    0;
      zones[i].velocities.byHi =    0;
      zones[i].sampleIndex     =   -1;
      zones[i].generators      = NULL;
      zones[i].modulators      = NULL;
      zones[i].genCount        =    0;
      zones[i].modCount        =    0;
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

    for (i = firstGenIdx + globalGenCount; i < lastGenIdx; i++) {
      if ((generators[i].sfGenOper != sfGenOper_keyRange) &&
          (generators[i].sfGenOper != sfGenOper_velRange) &&
          (generators[i].sfGenOper != sfGenOper_sampleID)) genCount++;
    }

    int modCount = lastModIdx - firstModIdx;

    if (genCount > 0) gens = new sfGenList[genCount];
    if (modCount > 0) mods = new sfModList[modCount];

    //if ((genCount > 0) || (modCount > 0))
    {
      uint16_t    z  = 0; // index into zones
      sfGenList * g  = gens;
      sfGenList * gg = &generators[bags[bagIdx].wGenNdx];
      sfModList * m  = mods;
      sfModList * mm = &modulators[bags[bagIdx].wModNdx];
      sfBag     * b  = &bags[bagIdx];

      i = 0;  // How many zone entries we have processed in bags

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
            zones[i].generators = g;
            while (count--) {
              switch (gg->sfGenOper) {
                case sfGenOper_keyRange:
                  zones[i].keys = gg->genAmount.ranges;
                  if (keys[zones[i].keys.byLo] == KEY_NOT_USED) {
                    for (int k = zones[i].keys.byLo; k <= zones[i].keys.byHi; k++) {
                      if (keys[k] != KEY_NOT_USED) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                      keys[k] = z;
                    }
                  }
                  break;
                case sfGenOper_velRange:
                  zones[i].velocities = gg->genAmount.ranges;
                  break;
                case sfGenOper_sampleID:
                  zones[i].sampleIndex = gg->genAmount.wAmount;
                  break;
                default:
                  *g++ = *gg;
                  zones[i].genCount++;
                  break;
              }
              gg++;
            }
            if (zones[i].genCount == 0) zones[i].generators = NULL;
          }
          else {
            // The generators list is not valid.
            while (count--) gg++;
          }
        }

        // ---- modulators ----

        count = b[1].wModNdx - b[0].wModNdx;

        if (count > 0) {
          zones[i].modulators = m;
          zones[i].modCount   = count;
          while (count--) *m++ = *mm++;
        }

        z++;       // Goto next zone
        b++; i++;  // ... and next bag
      }
    }

    loaded = true;
  }

  for (i = 0; i < zoneCount; i++) {
    if ((keysToLoad.byLo <= zones[i].keys.byHi) && (keysToLoad.byHi >= zones[i].keys.byLo)) {
      soundFont->loadSample(zones[i].sampleIndex);
      zones[i].synth.setDefaults(soundFont->samples[zones[i].sampleIndex]);
      zones[i].synth.setGens(globalZone.generators, globalZone.genCount);
      zones[i].synth.setGens(zones[i].generators,   zones[i].genCount);
      zones[i].synth.addGens(preset.getGlobalGens(),            preset.getGlobalGenCount());
      zones[i].synth.addGens(preset.getZoneGens(presetZoneIdx), preset.getZoneGenCount(presetZoneIdx));
      zones[i].synth.completeParams();
    }
  }

  // showZones();

  logger.DEBUG("Instrument %s (re) loaded.", name.c_str());
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

void Instrument::showZones()
{
  std::cerr << std::endl << "Zone List for Instrument [" << name << "]" << std::endl << std::endl;

  if ((globalZone.genCount > 0) || (globalZone.modCount > 0)) {
    std::cerr << "Globals :" << std::endl;
    if (globalZone.genCount > 0) {
      std::cerr << "  Generators:" << std::endl;
      for (int j = 0; j < globalZone.genCount; j++) {
        std::cerr << "    " << j << ": ";
        showGenerator(globalZone.generators[j]);
        std::cerr << std::endl;
      }
    }
    if (globalZone.modCount > 0) {
      std::cerr << "  Modulators:" << std::endl;
      for (int j = 0; j < globalZone.modCount; j++) {
        std::cerr << "    " << j << ": ";
        showModulator(globalZone.modulators[j]);
        std::cerr << std::endl;
      }
    }
  }
  for (int i = 0; i < zoneCount; i++) {
    std::cerr << i << ": " <<
      "keys ["         << +zones[i].keys.byLo << "-" << +zones[i].keys.byHi << "] " <<
      "velocities ["   << +zones[i].velocities.byLo << "-" << +zones[i].velocities.byHi << "] " <<
      "sample index [" <<  zones[i].sampleIndex << "] " <<
      "gen count ["    << +zones[i].genCount << "] " <<
      "mod count ["    << +zones[i].modCount << "] " << std::endl;
    if (zones[i].genCount > 0) {
      std::cerr << "  Generators:" << std::endl;
      for (int j = 0; j < zones[i].genCount; j++) {
        std::cerr << "    " << j << ": ";
        showGenerator(zones[i].generators[j]);
        std::cerr << std::endl;
      }
    }
    if (zones[i].modCount > 0) {
      std::cerr << "  Modulators:" << std::endl;
      for (int j = 0; j < zones[i].modCount; j++) {
        std::cerr << "    " << j << ": ";
        showModulator(zones[i].modulators[j]);
        std::cerr << std::endl;
      }
    }
  }

  std::cerr << std::endl << "[End]" << std::endl;
}

void Instrument::playNote(uint8_t note,
                          uint8_t velocity)
{
  uint16_t i = keys[note];

  if (i == KEY_NOT_USED) return;

  for (; i < zoneCount; i++) {

    if ((zones[i].sampleIndex != -1) &&
        (zones[i].keys.byLo <= note) &&
        (note <= zones[i].keys.byHi)) {
      if (((zones[i].velocities.byLo <= velocity) &&
          (velocity <= zones[i].velocities.byHi)) ||
          ((zones[i].velocities.byLo == 0) && (zones[i].velocities.byHi == 0))) {
        assert(zones[i].sampleIndex < (int16_t) soundFont->samples.size());
        assert(soundFont->samples[zones[i].sampleIndex] != NULL);
        poly->addVoice(
          soundFont->samples[zones[i].sampleIndex],
          note, velocity / 127.0f,
          zones[i].synth
        );
      }
    }
  }
}

void Instrument::stopNote(uint8_t note)
{
  int i = keys[note];
  if (i == KEY_NOT_USED) logger.ERROR("keys for note %d not found!", note);
  while ((i < zoneCount) &&
         (zones[i].sampleIndex != -1) &&
         (zones[i].keys.byLo <= note) &&
         (note <= zones[i].keys.byHi)) {
    // soundFont->samples[zones[i].sampleIndex]->stopNote(note);
    i++;
  }
}
