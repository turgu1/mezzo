#include <iostream>

#include "mezzo.h"
#include "instrument.h"

Instrument::Instrument(char      * instrumentName,
                       int         bagIdx,
                       int         bagCount,
                       sfBag     * bags,
                       sfGenList * generators,
                       sfModList * modulators)
{
  int i, count;

  name = instrumentName;
  logger.DEBUG("Instrument [%s] created.", name.c_str());

  loaded = false;

  if (bagCount == 0) return;

  zones     = new aZone[bagCount];
  zoneCount = bagCount;

  gens = NULL;
  mods = NULL;

  for (i = 0; i < zoneCount; i++) {
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

  globalZone.generators = NULL;
  globalZone.modulators = NULL;
  globalZone.genCount   =    0;
  globalZone.modCount   =    0;

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
     (generators[bags[bagIdx + 1].wGenNdx - 1].sfGenOper != sampleID));

  if (globalZonePresent) zoneCount--;

  // Count how many generators need to be allocated, considering
  // that some generator values will be retrieved as normal
  // instrument parameters.

  int genCount = 0;

  for (i = firstGenIdx; i < lastGenIdx; i++) {
    if ((generators[i].sfGenOper != keyRange) &&
        (generators[i].sfGenOper != velRange) &&
        (generators[i].sfGenOper != sampleID)) genCount++;
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
        (gg[count - 1].sfGenOper != sampleID)) {

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
        if (gg[count - 1].sfGenOper == sampleID) {
          z->generators = g;
          while (count--) {
            switch (gg->sfGenOper) {
              case keyRange:
                z->keys = gg->genAmount.ranges;
                break;
              case velRange:
                z->velocities = gg->genAmount.ranges;
                break;
              case sampleID:
                z->sampleIndex = gg->genAmount.wAmount;
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

  showZones();
}

Instrument::~Instrument()
{
  if (zones) delete [] zones;
  if (gens)  delete [] gens;
  if (mods)  delete [] mods;

  loaded = false;
}

bool Instrument::load()
{
  loaded = true;
  return true;
}

bool Instrument::unload()
{
  loaded = false;
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
  std::cerr << "[I:"  << m.index
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
