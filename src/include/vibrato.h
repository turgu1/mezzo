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

#ifndef _VIBRATO_
#define _VIBRATO_

#include <iomanip>

#include "mezzo.h"

class Vibrato
{
private:
  static bool allActive;

  Lfo      lfo;
  float    pitch;
  float    frequency;
  uint32_t delay;
  float    pitchSamples;

public:
  Vibrato() { pitch = 0.0f; frequency = 8.176f; delay = centsToSampleCount(-12000); }

  inline void setup(uint8_t note) {
    // The chosen phase is to get a smooth transition at the beginning of the vibrato integration.
    lfo = Lfo(frequency, 3.0f * M_PI / 2.0f);
    pitchSamples = log2(noteFrequency(note) * pitch / 100.0f);
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive()    { return allActive;              }

  inline void setPitch(int16_t p)     { pitch      = ((float) p); }
  inline void addToPitch(int16_t p)   { pitch     += ((float) p); }

  inline void setDelay(int16_t d)     { delay      = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDelay(int16_t d)   { delay     *= (d == -32768) ? 1 : centsToRatio(d); }

  inline void setFrequency(float f)   { frequency  = centsToFreq(f); }
  inline void addToFrequency(float f) { frequency *= centsToRatio(f); }

  inline float nextValue(uint32_t pos)
  {
    if (allActive && (pitch  != 0.0f) && (frequency != 0.0f)) {
      return (pos < delay) ? 0.0f : (pitchSamples + (lfo.nextValue() * pitchSamples));
    }
    else {
      return 0.0f;
    }
  }

  void showStatus(int spaces)
  {
    using namespace std;

    cout << setw(spaces) << ' '
         <<"Vibrato: " << ((allActive && (pitch  != 0.0f) && (frequency != 0.0f)) ? "Active" : "Inactive")
         << " [Delay:" << delay
         << " Pitch:"  << pitch
         << " Freq:"   << frequency
         << "] ";

    lfo.showStatus(2);
  }

};

#endif