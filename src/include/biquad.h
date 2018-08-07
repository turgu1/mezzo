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

#ifndef _BIQUAD_
#define _BIQUAD_

#include "mezzo.h"

// The algorithm come from the following url:
//
//    http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
//
// Two biQuad IIR parameters have been used:
// 1. The biQuad parameters computation as a by-product of the following:
//
//   http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//
// 2. The Butterworth parameters computation comes from the book "The Audio
// Programming Book" by Richard Boulanger and Al., table 6.1
//
// Not sure if the end-state of it is the right one. Seems to be
// similar to the one used in the Polyphone program.

class BiQuad
{
private:
  bool active;              // This instance
  static bool allActive;  // For all instances

  float initialFc;
  float initialQ;
  float a1, a2, b0, b1;
  float x1, x2, y1, y2;
  float gain;

public:
  BiQuad()
  {
    // initialFc = centsToRatio(13500) / config.samplingRate;
    initialFc = 13500;
    initialQ  =  0.0f;
    active    = false;
    gain      =  1.3f;
  }

  inline void setInitialFc  (int16_t fc) { initialFc  = fc; }
  inline void addToInitialFc(int16_t fc) { initialFc += fc; }
  inline void setInitialQ   (int16_t  q) { initialQ   =  q; }
  inline void addToInitialQ (int16_t  q) { initialQ  +=  q; }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive   () { return allActive;              }

  void setup();
  void filter(sampleRecord & src, uint16_t length);
  void showStatus(int spaces);
};

#endif