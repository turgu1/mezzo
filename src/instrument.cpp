#include "mezzo.h"
#include "instrument.h"

Instrument::Instrument(std::string & instrument_name)
{
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