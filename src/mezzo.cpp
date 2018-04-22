#include "mezzo.h"

#include "soundfont2.h"

Mezzo::Mezzo(std::string & sf2_filename)
{
  if (soundFont) delete soundFont;
  soundFont = new SoundFont2(sf2_filename);
  
}

Mezzo::~Mezzo()
{
  if (soundFont) delete soundFont;  
}