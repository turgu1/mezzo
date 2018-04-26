#include "copyright.h"

#include "mezzo.h"
#include "preset.h"

#include "soundfont2.h"

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
  for (i = 0; i < zoneCount; i++) {
    assert(soundFont != NULL);
    assert(soundFont->instruments[zones[i].instrumentIndex] != NULL);
    soundFont->instruments[zones[i].instrumentIndex]->unload();
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

  if (bagCount == 0) return false;

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
     (generators[bags[bagIdx + 1].wGenNdx - 1].sfGenOper != instrumentID));

  if (globalZonePresent) zoneCount--;

  // Count how many generators need to be allocated, considering
  // that some generator values will be retrieved as normal
  // instrument parameters.

  int genCount = 0;

  for (i = firstGenIdx; i < lastGenIdx; i++) {
    if ((generators[i].sfGenOper != keyRange) &&
        (generators[i].sfGenOper != velRange) &&
        (generators[i].sfGenOper != instrumentID)) genCount++;
  }

  int modCount = lastModIdx - firstModIdx;

  if (genCount > 0) gens = new sfGenList[genCount];
  if (modCount > 0) mods = new sfModList[modCount];

  if ((genCount > 0) || (modCount > 0)) {

    aZone     * z  = zones;
    sfGenList * g  = gens;
    sfGenList * gg = &generators[bags[bagIdx].wGenNdx];
    sfModList * m  = mods;
    sfModList * mm = &modulators[bags[bagIdx].wModNdx];
    sfBag     * b  = &bags[bagIdx];

    i = 0;  // How many zone entries we have processed in bags

    // ---- globalZone ----

    // global gens

    count = b[1].wGenNdx - b[0].wGenNdx;
    if ((count > 0) &&
        (gg[count - 1].sfGenOper != instrumentID)) {

      // There is globals gens so get the gens...
      globalZone.generators = g;
      globalZone.genCount   = count;

      while (count--) *g++ = *gg++;

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
        if (gg[count - 1].sfGenOper == instrumentID) {
          z->generators = g;
          while (count--) {
            switch (gg->sfGenOper) {
              case keyRange:
                z->keys = gg->genAmount.ranges;
                if (keys[z->keys.byLo] == NULL) {
                  for (int k = z->keys.byLo; k <= z->keys.byHi; k++) {
                    if (keys[k] != NULL) logger.ERROR("MIDI Keys redondancies in zones for key %d.", k);
                    keys[k] = z;
                  }
                }
                break;
              case velRange:
                z->velocities = gg->genAmount.ranges;
                break;
              case instrumentID:
                z->instrumentIndex = gg->genAmount.wAmount;
                break;
              default:
                *g++ = *gg;
                z->genCount++;
                break;
            }
            gg++;
          }
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

  loaded = true;
  return true;
}
