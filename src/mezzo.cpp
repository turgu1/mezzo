#include "copyright.h"

#include "mezzo.h"

#include "soundfont2.h"

Mezzo::Mezzo()
{
  setNewHandler(outOfMemory);

  if (soundFont) delete soundFont;
  soundFont = new SoundFont2(soundFontFilename);
  
  soundFont->loadPreset(0);
}

Mezzo::~Mezzo()
{
  if (soundFont) delete soundFont;  
}

void Mezzo::outOfMemory()
{
  logger.FATAL("Mezzo: Unable to allocate memory.");
}

