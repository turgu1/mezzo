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

#ifndef _POLY_
#define _POLY_

#include "mezzo.h"

class Poly : public NewHandlerSupport<Poly> {

 private:
  voicep       voices;
  volatile int voiceCount;
  volatile int maxVoiceCount;

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

 public:
   Poly();
  ~Poly();

  int mixer(frameRecord & buff);

  void   inactivateAllVoices();
  void   showState();
  voicep firstVoice();
  voicep nextVoice(voicep prev);
  void   addVoice(samplep s, uint8_t note, float gain, Synthesizer & synth,
                  Preset & preset, uint16_t presetZoneIdx);

  voicep nextAvailable();
  void   noteOff(char note, bool pedal_on);
  void   voicesSustainOff();
  voicep removeVoice(voicep v, voicep prev);
  //int    getFrames(voicep v, buffp buff, int count);
  void   monitorCount();

  inline voicep getVoices() { return voices; }
  inline void   decVoiceCount() { voiceCount--; }
  inline int    getVoiceCount() { return voiceCount; }

  bool showVoicePlayingState() { return Voice::togglePlayingState(); };
};

void   stopThreads();
void * samplesFeeder(void * args);
void * voicesFeeder1(void * args);
void * voicesFeeder2(void * args);

#endif // POLY_H
