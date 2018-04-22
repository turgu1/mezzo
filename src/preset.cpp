#include "mezzo.h"
#include "preset.h"

Preset::Preset(char * presetName, uint16_t midi, uint16_t bank)
{
  name    = presetName;
  midiNbr = midi;
  bankNbr = bank;
  loaded  = false;
  
  logger.DEBUG("Preset [%s] created.", name.c_str());
}

Preset::~Preset()
{
  loaded = false;
}

bool Preset::load()
{
  loaded = true;
  return true;
}

bool Preset::unload()
{
  loaded = false;
  return true;
}