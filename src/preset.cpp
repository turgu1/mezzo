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

  for (int i = 0; i < 128; i++) keys[i] = NULL;
  keyShortCutPresent = false;
  velocitiesPresent = false;
  
  loaded  = false;
}

Preset::~Preset()
{
  if (loaded) unload();
}

void Preset::outOfMemory()
{
  logger.FATAL("Preset: Unable to allocate memory.");
}

bool Preset::unload()
{
  if (!loaded) return false;
  assert(soundFont != NULL);

  for (int i = 0; i < zoneCount; i++) {
    if (zones[i].instrumentIndex >= 0) {
      assert(zones[i].instrumentIndex < (int) soundFont->instruments.size());
      assert(soundFont->instruments[zones[i].instrumentIndex] != NULL);
      soundFont->instruments[zones[i].instrumentIndex]->unload();
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
  int i, count;

  if (loaded) return true;

  if (bagCount == 0) {
    logger.DEBUG("Bag Count is 0 for preset %s", name.c_str());
    return false;
  }

  // We get one more as an end of list indicator (instrumentIndex will be -1)

  zones     = new aZone[bagCount + 1];
  zoneCount = bagCount;

  for (i = 0; i <= zoneCount; i++) {
    zones[i].keys.byHi       =    0;
    zones[i].keys.byLo       =    0;
    zones[i].velocities.byLo =    0;
    zones[i].velocities.byHi =    0;
    zones[i].instrumentIndex =   -1;
    zones[i].generators      = NULL;
    zones[i].modulators      = NULL;
    zones[i].genCount        =    0;
    zones[i].modCount        =    0;
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

  for (i = firstGenIdx + globalGenCount; i < lastGenIdx; i++) {
    if ((generators[i].sfGenOper != sfGenOper_keyRange) &&
        (generators[i].sfGenOper != sfGenOper_velRange) &&
        (generators[i].sfGenOper != sfGenOper_instrumentID)) genCount++;
  }

  int modCount = lastModIdx - firstModIdx;

  if (genCount > 0) gens = new sfGenList[genCount];
  if (modCount > 0) mods = new sfModList[modCount];

  //if ((genCount > 0) || (modCount > 0))
  {
    aZone     * z  = zones;
    sfGenList * g  = gens;
    sfGenList * gg = &generators[bags[bagIdx].wGenNdx];
    sfModList * m  = mods;
    sfModList * mm = &modulators[bags[bagIdx].wModNdx];
    sfBag     * b  = &bags[bagIdx];

    i = 0;  // How many zone entries we have processed in bags

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
          z->generators = g;
          while (count--) {
            switch (gg->sfGenOper) {
              case sfGenOper_keyRange:
                z->keys = gg->genAmount.ranges;
                if (keys[z->keys.byLo] == NULL) {
                  for (int k = z->keys.byLo; k <= z->keys.byHi; k++) {
                    if (keys[k] != NULL) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                    keys[k] = z;
                    keyShortCutPresent = true;
                  }
                }
                break;
              case sfGenOper_velRange:
                z->velocities = gg->genAmount.ranges;
                velocitiesPresent = true;
                break;
              case sfGenOper_instrumentID:
                z->instrumentIndex = gg->genAmount.wAmount;
                break;
              default:
                *g++ = *gg;
                z->genCount++;
                break;
            }
            gg++;
          }
          std::cout << std::endl;
          if (z->genCount == 0) z->generators = NULL;
        }
        else {
          // The generators list is not valid.
          while (count--) gg++;
        }
      }

      // ---- modulators ----

      count = b[1].wModNdx - b[0].wModNdx;

      if (count > 0) {
        z->modulators = m;
        z->modCount   = count;
        while (count--) *m++ = *mm++;
      }

      z++;       // Goto next zone
      b++; i++;  // ... and next bag
    }
  }

  // Load instruments

  for (i = 0; i < zoneCount; i++) {
    assert(soundFont != NULL);
    soundFont->loadInstrument(zones[i].instrumentIndex, zones[i].keys);
  }

  showZones();
  
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
  std::cerr << "ModSrc ";
  showModInfo(m.sfModSrcOper);
  std::cerr << " Dest [" << generatorsDesc[m.sfModDestOper].name << "]"
            << " Amount [" << m.modAmount << "]"
            << " ModAmtSrc ";
  showModInfo(m.sfModAmtSrcOper);
  std::cerr << " Transform [" << (m.sfModTransOper == 0 ? "Linear" : "AbsValue") << "]";
}

void Preset::showZones()
{
  std::cerr << std::endl << "Zone List for Preset [" << name << "]" << std::endl << std::endl;

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
      "instrument index [" <<  zones[i].instrumentIndex << "] " <<
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

void Preset::playNote(uint8_t note, uint8_t velocity) {
  aZone * z = keys[note];
  if (z) {
    while ((z->instrumentIndex != -999) &&
           (z->keys.byLo <= note) &&
           (note <= z->keys.byHi)) {
      if ((z->instrumentIndex != -1) &&
          (z->velocities.byLo <= velocity) &&
          (velocity <= z->velocities.byHi)) {
        soundFont->instruments[z->instrumentIndex]->playNote(note, velocity);
      }
      z++;
    }
  }
  else if (!keyShortCutPresent) {
    z = zones;
    assert(z != NULL);
    if (velocitiesPresent) {
      while (z->instrumentIndex != -999) {
        if ((z->instrumentIndex != -1) &&
            (z->velocities.byLo <= velocity) &&
            (velocity <= z->velocities.byHi)) {
          soundFont->instruments[z->instrumentIndex]->playNote(note, velocity);
        }
        z++;
      }
    }
    else {
      while ((z->instrumentIndex != -999) &&
             (z->instrumentIndex == -1)) {
        z++;         
      }
      if (z->instrumentIndex != -999) {
        soundFont->instruments[z->instrumentIndex]->playNote(note, velocity);
      }
    }
  }
}

void Preset::stopNote(uint8_t note) {
  aZone * z = keys[note];
  while (z && (z->instrumentIndex != -999) &&
         (z->keys.byLo <= note) &&
         (note <= z->keys.byHi)) {
    if (z->instrumentIndex != -1) {
      soundFont->instruments[z->instrumentIndex]->stopNote(note);
    }
    z++;
  }
}
