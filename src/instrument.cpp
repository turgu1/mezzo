#include "mezzo.h"
#include "instrument.h"

Instrument::Instrument(char      * instrumentName, 
                       int         bagIdx, 
                       int         bagCount, 
                       sfBag     * bags, 
                       sfGenList * generators, 
                       sfModList * modulators)
{
  name = instrumentName;
  logger.DEBUG("Instrument [%s] created.", name.c_str());
  loaded = false;
}

Instrument::~Instrument()
{
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