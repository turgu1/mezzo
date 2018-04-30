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
  int count;

  if (bagCount == 0) return false;

  if (!loaded) {

    // We get one more as an end of list indicator (sampleIndex will be -1)

    zones     = new aZone[bagCount + 1];
    zoneCount = bagCount;

    for (uint16_t zIdx = 0; zIdx <= zoneCount; zIdx++) {
      zones[zIdx].keys.byHi       =    0;
      zones[zIdx].keys.byLo       =    0;
      zones[zIdx].velocities.byLo =    0;
      zones[zIdx].velocities.byHi =    0;
      zones[zIdx].sampleIndex     =   -1;
      zones[zIdx].generators      = NULL;
      zones[zIdx].modulators      = NULL;
      zones[zIdx].genCount        =    0;
      zones[zIdx].modCount        =    0;
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
      uint16_t    zIdx  = 0; // index into zones
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
            zones[zIdx].generators = g;
            while (count--) {
              switch (gg->sfGenOper) {
                case sfGenOper_keyRange:
                  zones[zIdx].keys = gg->genAmount.ranges;
                  if (keys[zones[zIdx].keys.byLo] == KEY_NOT_USED) {
                    for (int k = zones[zIdx].keys.byLo; k <= zones[zIdx].keys.byHi; k++) {
                      if (keys[k] != KEY_NOT_USED) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                      keys[k] = zIdx;
                    }
                  }
                  break;
                case sfGenOper_velRange:
                  zones[zIdx].velocities = gg->genAmount.ranges;
                  break;
                case sfGenOper_sampleID:
                  zones[zIdx].sampleIndex = gg->genAmount.wAmount;
                  break;
                default:
                  *g++ = *gg;
                  zones[zIdx].genCount++;
                  break;
              }
              gg++;
            }
            if (zones[zIdx].genCount == 0) zones[zIdx].generators = NULL;
          }
          else {
            // The generators list is not valid.
            while (count--) gg++;
          }
        }

        // ---- modulators ----

        count = b[1].wModNdx - b[0].wModNdx;

        if (count > 0) {
          zones[zIdx].modulators = m;
          zones[zIdx].modCount   = count;
          while (count--) *m++ = *mm++;
        }

        if (zones[zIdx].sampleIndex != -1) {
          zIdx++;       // Goto next zone
        } 
        else {
          zoneCount--;
        }
        b++; i++;  // ... and next bag
      }
    }

    loaded = true;
  }

  for (uint16_t zIdx = 0; zIdx < zoneCount; zIdx++) {
    if ((keysToLoad.byLo <= zones[zIdx].keys.byHi) && (keysToLoad.byHi >= zones[zIdx].keys.byLo)) {
      if (soundFont->loadSample(zones[zIdx].sampleIndex)) {
        zones[zIdx].synth.setDefaults(soundFont->samples[zones[zIdx].sampleIndex]);
        zones[zIdx].synth.setGens(globalZone.generators, globalZone.genCount);
        zones[zIdx].synth.setGens(zones[zIdx].generators,   zones[zIdx].genCount);
        zones[zIdx].synth.addGens(preset.getGlobalGens(),            preset.getGlobalGenCount());
        zones[zIdx].synth.addGens(preset.getZoneGens(presetZoneIdx), preset.getZoneGenCount(presetZoneIdx));
        zones[zIdx].synth.completeParams();
      }
      else {
        logger.DEBUG("Unable to load sample for zones[%d] sampleIndex %d (zoneCount: %d)", zIdx, zones[zIdx].sampleIndex, zoneCount);
        showZone(zIdx);
      }
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

void Instrument::showZone(uint16_t zIdx)
{
  using namespace std;
  
  cerr << "Zone "<< zIdx << ": " <<
    "keys ["         << +zones[zIdx].keys.byLo << "-" << +zones[zIdx].keys.byHi << "] " <<
    "velocities ["   << +zones[zIdx].velocities.byLo << "-" << +zones[zIdx].velocities.byHi << "] " <<
    "sample index [" <<  zones[zIdx].sampleIndex << "] " <<
    "gen count ["    << +zones[zIdx].genCount << "] " <<
    "mod count ["    << +zones[zIdx].modCount << "] " << endl;
  cerr << "  ";
  soundFont->samples[zones[zIdx].sampleIndex]->showState();
  cerr << "  ";
  zones[zIdx].synth.showParams();
  if (zones[zIdx].genCount > 0) {
    cerr << "  Generators:" << endl;
    for (int j = 0; j < zones[zIdx].genCount; j++) {
      cerr << "    " << j << ": ";
      showGenerator(zones[zIdx].generators[j]);
      cerr << endl;
    }
  }
  if (zones[zIdx].modCount > 0) {
    cerr << "  Modulators:" << endl;
    for (int j = 0; j < zones[zIdx].modCount; j++) {
      cerr << "    " << j << ": ";
      showModulator(zones[zIdx].modulators[j]);
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
  for (int zIdx = 0; zIdx < zoneCount; zIdx++) showZone(zIdx);

  cerr << endl << "[End]" << endl;
}

void Instrument::playNote(uint8_t note,
                          uint8_t velocity)
{
  uint16_t zIdx = keys[note];

  if (zIdx == KEY_NOT_USED) return;

  for (; zIdx < zoneCount; zIdx++) {

    if ((zones[zIdx].sampleIndex != -1) &&
        (zones[zIdx].keys.byLo <= note) &&
        (note <= zones[zIdx].keys.byHi)) {
      if (((zones[zIdx].velocities.byLo <= velocity) &&
          (velocity <= zones[zIdx].velocities.byHi)) ||
          ((zones[zIdx].velocities.byLo == 0) && (zones[zIdx].velocities.byHi == 0))) {
        assert(zones[zIdx].sampleIndex < (int16_t) soundFont->samples.size());
        assert(soundFont->samples[zones[zIdx].sampleIndex] != NULL);
        poly->addVoice(
          soundFont->samples[zones[zIdx].sampleIndex],
          note, velocity / 127.0f,
          zones[zIdx].synth
        );
      }
    }
  }
}

void Instrument::stopNote(uint8_t note)
{
  int zIdx = keys[note];
  if (zIdx == KEY_NOT_USED) logger.ERROR("keys for note %d not found!", note);
  while ((zIdx < zoneCount) &&
         (zones[zIdx].sampleIndex != -1) &&
         (zones[zIdx].keys.byLo <= note) &&
         (note <= zones[zIdx].keys.byHi)) {
    // soundFont->samples[zones[zIdx].sampleIndex]->stopNote(note);
    zIdx++;
  }
}
