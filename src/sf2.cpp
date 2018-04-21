#include "mezzo.h"
#include "sf2.h"

SoundFont2::SoundFont2(std::string & sf2_filename)
{
  file.open(sf2_filename);
  if (!file.is_open()) {
    logger.ERROR("Unable to open file %s.", sf2_filename.c_str());
  }
  else {
    data = file.data();
    
    loaded = retrieve_instrument_list() && retrieve_preset_list();
  }
}

SoundFont2::~SoundFont2()
{
  if (file.is_open()) file.close();
}

bool SoundFont2::load_instrument(std::string & instrument_name)
{
  
  return true;
}

bool SoundFont2::load_instrument(int instrument_index)
{
  
  return true;
}

bool SoundFont2::load_preset(std::string & preset_name)
{
  
  return true;
}

bool SoundFont2::load_preset(int preset_index)
{
  
  return true;
}

bool SoundFont2::retrieve_instrument_list()
{
  
  return true;
} 

bool SoundFont2::retrieve_preset_list()
{
  
  return true;
}



