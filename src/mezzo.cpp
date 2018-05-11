#include "copyright.h"
#include <iostream>
#include <fstream>

#include "mezzo.h"

#include "soundfont2.h"

Mezzo::Mezzo()
{
  setNewHandler(outOfMemory);

  if (soundFont) delete soundFont;
  soundFont = new SoundFont2(config.soundFontFilename);

  soundFont->loadMidiPreset(0, 0);

  reverb    = new Reverb();
  poly      = new Poly();
  equalizer = new Equalizer();
  sound     = new Sound();
  midi      = new Midi();

  sound->conti();

  binFile.open("data.bin", std::ios::out | std::ios::binary);
  logger.INFO("Ready!");
}

Mezzo::~Mezzo()
{
  binFile.close();
  
  if (soundFont) delete soundFont;

  delete midi;
  delete sound;
  delete poly;
  delete reverb;
  delete equalizer;

  logger.INFO("Max number of voices mixed at once: %d.", maxVoicesMixed);

  // logger.INFO("Max volume: %8.2f.", maxVolume);

  logger.INFO("Max duration of mixer function: %ld nsec (%.0f Hz).",
              mixerDuration, 1000000000.0 / mixerDuration);
  logger.INFO("Min duration of reverb function: %ld nsec (%.0f Hz).",
              reverbMinDuration, 1000000000.0 / reverbMinDuration);
  logger.INFO("Max duration of reverb function: %ld nsec (%.0f Hz).",
              reverbMaxDuration, 1000000000.0 / reverbMaxDuration);
}

void Mezzo::outOfMemory()
{
  logger.FATAL("Mezzo: Unable to allocate memory.");
}
