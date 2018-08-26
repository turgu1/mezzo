// Notice
// ------
//
// This file is part of the Mezzo SoundFont2 Sampling Based Synthesizer:
//
//     https://github.com/turgu1/mezzo
//
// Simplified BSD License
// ----------------------
//
// Copyright (c) 2018, Guy Turcotte
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//

#include <iostream>
#include <fstream>

#include "mezzo.h"

#define show(v)

Mezzo::Mezzo()
{
  setNewHandler(outOfMemory);

  if (soundFont) delete soundFont;
  soundFont = new SoundFont2(config.soundFontFilename);

  soundFont->loadMidiPreset(0, 0);

  show("reverb");    reverb    = new Reverb();
  show("poly");      poly      = new Poly();
  show("equalizer"); equalizer = new Equalizer();
  show("sound");     sound     = new Sound();
  show("midi");      midi      = new Midi();
  show("metronome"); metronome = new Metronome();
  
  show("conti");     sound->conti();

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
