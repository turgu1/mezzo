#include "mezzo.h"
#include "preset.h"

Preset::Preset(std::string & preset_name)
{
  loaded = false;
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