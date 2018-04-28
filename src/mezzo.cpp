#include "copyright.h"

#include "mezzo.h"

#include "soundfont2.h"

Mezzo::Mezzo()
{
  setNewHandler(outOfMemory);

  if (soundFont) delete soundFont;
  soundFont = new SoundFont2(config.soundFontFilename);

  soundFont->loadPreset(0);

  reverb    = new Reverb();
  poly      = new Poly();
  equalizer = new Equalizer();
  sound     = new Sound();
  midi      = new Midi();

  sound->conti();

  logger.INFO("Ready!");
}

Mezzo::~Mezzo()
{
  if (soundFont) delete soundFont;
}

void Mezzo::outOfMemory()
{
  logger.FATAL("Mezzo: Unable to allocate memory.");
}
