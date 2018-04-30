#include "copyright.h"

#include "mezzo.h"

Preset::Preset(char * presetName,
               uint16_t midi,
               uint16_t bank,
               uint16_t bagIndex,
               uint16_t bagQty)
{
  setNewHandler(outOfMemory);

  name    = presetName;
  midiNbr = midi;
  bankNbr = bank;

  bagIdx   = bagIndex;
  bagCount = bagQty;

  nextMidiPreset = NULL;

  init();

  logger.DEBUG("Preset [%s] created.", name.c_str());
}

void Preset::init()
{
  gens  = NULL;
  mods  = NULL;
  zones = NULL;

  zoneCount = 0;

  globalZone.generators = NULL;
  globalZone.modulators = NULL;
  globalZone.genCount   =    0;
  globalZone.modCount   =    0;

  for (int i = 0; i < 128; i++) keys[i] = KEY_NOT_USED;

  keyShortCutPresent = false;
  velocitiesPresent = false;

  loaded  = false;
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

  for (int zIdx = 0; zIdx < zoneCount; zIdx++) {
    if (zones[zIdx].instrumentIndex >= 0) {
      assert(zones[zIdx].instrumentIndex < (int) soundFont->instruments.size());
      assert(soundFont->instruments[zones[zIdx].instrumentIndex] != NULL);
      soundFont->instruments[zones[zIdx].instrumentIndex]->unload();
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

  for (int zIdx = 0; zIdx <= zoneCount; zIdx++) {
    zones[zIdx].keys.byHi       =    0;
    zones[zIdx].keys.byLo       =    0;
    zones[zIdx].velocities.byLo =    0;
    zones[zIdx].velocities.byHi =    0;
    zones[zIdx].instrumentIndex =   -1;
    zones[zIdx].generators      = NULL;
    zones[zIdx].modulators      = NULL;
    zones[zIdx].genCount        =    0;
    zones[zIdx].modCount        =    0;
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
    uint16_t    zIdx = 0;   // Index into zones
    sfGenList *    g = gens;
    sfGenList *   gg = &generators[bags[bagIdx].wGenNdx];
    sfModList *    m = mods;
    sfModList *   mm = &modulators[bags[bagIdx].wModNdx];
    sfBag     *    b = &bags[bagIdx];

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
          zones[zIdx].generators = g;
          while (count--) {
            switch (gg->sfGenOper) {
              case sfGenOper_keyRange:
                zones[zIdx].keys = gg->genAmount.ranges;
                if (keys[zones[zIdx].keys.byLo] == KEY_NOT_USED) {
                  for (int k = zones[zIdx].keys.byLo; k <= zones[zIdx].keys.byHi; k++) {
                    if (keys[k] != KEY_NOT_USED) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                    keys[k] = zIdx;
                    keyShortCutPresent = true;
                  }
                }
                break;
              case sfGenOper_velRange:
                zones[zIdx].velocities = gg->genAmount.ranges;
                velocitiesPresent = true;
                break;
              case sfGenOper_instrumentID:
                zones[zIdx].instrumentIndex = gg->genAmount.wAmount;
                uint16_t k;
                for (k = 0; k < instruments.size(); k++) {
                  if (instruments[k]->index == zones[zIdx].instrumentIndex) break;
                }
                if (k >= instruments.size()) {
                  presetInstrument * pi = new presetInstrument;
                  assert(pi != NULL);
                  pi->index = zones[zIdx].instrumentIndex;
                  Instrument * inst = soundFont->getInstrument(pi->index);
                  assert(inst != NULL);
                  pi->name = inst->getName();
                  instruments.push_back(pi);
                }
                break;
              default:
                *g++ = *gg;
                zones[zIdx].genCount++;
                break;
            }
            gg++;
          }
          std::cout << std::endl;
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

      if (zones[zIdx].instrumentIndex != -1) {
        zIdx++;       // Goto next zone
      } 
      else {
        zoneCount--;
      }
      b++; i++;  // ... and next bag
    }
  }

  // Load instruments

  for (uint16_t zIdx = 0; zIdx < zoneCount; zIdx++) {
    assert(soundFont != NULL);
    if ((zones[zIdx].keys.byHi == 0) && (zones[zIdx].keys.byLo == 0)) {
      zones[zIdx].keys.byHi = 127;
    };

    if ((zones[zIdx].velocities.byHi == 0) && (zones[zIdx].velocities.byLo == 0)) {
      zones[zIdx].velocities.byHi = 127;
    };

    soundFont->loadInstrument(zones[zIdx].instrumentIndex,
                              zones[zIdx].keys,
                              *this, zIdx);
  }

  // showZones();

  logger.DEBUG("Preset %s loaded.", name.c_str());

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
  cerr << " Dest [" << generatorsDesc[m.sfModDestOper].name << "]"
            << " Amount [" << m.modAmount << "]"
            << " ModAmtSrc ";
  showModInfo(m.sfModAmtSrcOper);
  cerr << " Transform [" << (m.sfModTransOper == 0 ? "Linear" : "AbsValue") << "]";
}

void Preset::showZone(uint16_t zIdx)
{
  using namespace std;
  
  cerr << "Zone " << zIdx << ": " <<
    "keys ["         << +zones[zIdx].keys.byLo << "-" << +zones[zIdx].keys.byHi << "] " <<
    "velocities ["   << +zones[zIdx].velocities.byLo << "-" << +zones[zIdx].velocities.byHi << "] " <<
    "instrument index [" <<  zones[zIdx].instrumentIndex << "] " <<
    "gen count ["    << +zones[zIdx].genCount << "] " <<
    "mod count ["    << +zones[zIdx].modCount << "] " << endl;
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
  for (uint16_t zIdx = 0; zIdx < zoneCount; zIdx++) showZone(zIdx);

  cerr << endl << "[End]" << endl;
}

void Preset::playNote(uint8_t note, uint8_t velocity)
{
  for (uint16_t zIdx = 0; zIdx < zoneCount; zIdx++) {
    if (zones[zIdx].instrumentIndex != -1) {
      if ((zones[zIdx].keys.byLo <= note) && (note <= zones[zIdx].keys.byHi)) {
        if ((zones[zIdx].velocities.byLo <= velocity) &&
            (velocity <= zones[zIdx].velocities.byHi)) {
          soundFont->instruments[zones[zIdx].instrumentIndex]->playNote(
            note, velocity
          );
        }
        else if ((zones[zIdx].velocities.byLo == 0) &&
                 (zones[zIdx].velocities.byHi == 0)) {
          soundFont->instruments[zones[zIdx].instrumentIndex]->playNote(
            note, velocity
          );
        }
      }
      else if ((zones[zIdx].keys.byLo == 0) && (zones[zIdx].keys.byHi == 0)) {
        if ((zones[zIdx].velocities.byLo <= velocity) &&
            (velocity <= zones[zIdx].velocities.byHi)) {
          soundFont->instruments[zones[zIdx].instrumentIndex]->playNote(
            note, velocity
          );
        }
        else if ((zones[zIdx].velocities.byLo == 0) &&
                 (zones[zIdx].velocities.byHi == 0)) {
          soundFont->instruments[zones[zIdx].instrumentIndex]->playNote(
            note, velocity
          );
        }
      }
    }
  }
}

void Preset::stopNote(uint8_t note) {
  uint16_t zIdx = keys[note];
  while ((zIdx < zoneCount) &&
         (zones[zIdx].keys.byLo <= note) &&
         (note <= zones[zIdx].keys.byHi)) {
    if (zones[zIdx].instrumentIndex != -1) {
      soundFont->instruments[zones[zIdx].instrumentIndex]->stopNote(note);
    }
    zIdx++;
  }
}
