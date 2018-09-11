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

#include <unistd.h>

#include "mezzo.h"

#include "metticks.h"

static pthread_t ticker;

void *   metTicker(void * args)
{
  while (metronome->isActive()) {
    usleep(60000000 / metronome->getBeatsPerMinute());
    metronome->setTickStart(true);
  }
  
  pthread_exit(NULL);
}

void Metronome::start()
{
  if (!active) {
    active    = true;
    tickStart = false;
    tickCount = 0;
    if (pthread_create(&ticker, NULL, metTicker, NULL)) {
      logger.FATAL("Unable to start metTicker thread.");
    }
  }
}

void Metronome::stopWait()
{
  if (active) {
    active = false;
    pthread_join(ticker, NULL);
  }
}

void Metronome::process(frameRecord & buff)
{
  if (active) {
    if (tickStart) {
      tickEnd   = false;
      tickStart = false;
      pos = 0;
      if ((tickCount % beatsPerMeasure) == 0) {
        sound = metTick1.data();
        soundSize = metTick1.size();
      }
      else {
        sound = metTick2.data();
        soundSize = metTick2.size();        
      }
    }

    if (!tickEnd) {
      int len = MIN((soundSize - pos) / 2 , buff.size());
      float gain = config.masterVolume * 0.5;
      for (int i = 0; i < len; i++) {
        buff[i].left  += sound[pos++] * gain;
        buff[i].right += sound[pos++] * gain;
      }
      tickEnd = pos >= soundSize;
      if (tickEnd) tickCount++;
    }
  }
}

Metronome::Metronome()
{
  active          = false;
  tickStart       = false;
  tickEnd         = true;
  tickCount       = 0;
  beatsPerMinute  = config.metBeatsPerMinute;
  beatsPerMeasure = config.metBeatsPerMeasure;
}