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

#ifndef _EQUALIZER_
#define _EQUALIZER_

// A very simple form of a 7 bands equalizer. 
// IIR Filter algorithm and equalizer coefficients from the following book:
//
// Paul M. Embree, C Algorithms for Real-Time DSP, 1995
//
// TODO: Neon Intrinsics Optimization

#define BAND_COUNT 7

class Equalizer {

 private:
  static float  leftHist[BAND_COUNT][2];
  static float rightHist[BAND_COUNT][2];
  static float      gain[BAND_COUNT];

  static const float bpf[BAND_COUNT][5];  // Bandpass Filter Coefficients

  float  iirFilter(float input, const float *coef, int n, float *history);
  void adjustGain(char c);

  // Optimized version (when n == 1 and coefficients 4 and 5 are 0 and -1)
  inline float iirFilter2(float input, const float *coef, float *history)
  {
    const float *coefPtr;
    float *hist1Ptr, *hist2Ptr;
    float output, newHist, history1, history2;
  
    coefPtr = coef;

    hist1Ptr = history;
    hist2Ptr = hist1Ptr + 1;

    output = input * *coefPtr++;

    history1 = *hist1Ptr;
    history2 = *hist2Ptr;
    
    output -= history1 * *coefPtr++;
    newHist = output - history2 * *coefPtr;

    output  = newHist;
    output -= history2;

    *hist2Ptr = *hist1Ptr;
    *hist1Ptr = newHist;

    return output;
  }

 public:
   Equalizer();
  ~Equalizer();

  void process(buffp buff, int frameCount);
  void interactiveAdjust();
};

#endif
