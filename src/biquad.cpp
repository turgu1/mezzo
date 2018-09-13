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

#include "mezzo.h"

bool BiQuad::allActive = true;

void BiQuad::setup()
{
  // 3.01 DB attenuation
  initialQ = centibelToRatio(initialQ - 30.1);
}

void BiQuad::computeParams(float freq, float Q)
{
  float theta = 2. * M_PI * centsToFreq(freq) / config.samplingRate;

  if (Q == 0) {
    active = false;
  }
  else {
    float d = 1. / initialQ;
    float dTmp = d * Utils::highSin(theta).toFloat() / 2.;
    if (dTmp == -1) {
      active = false;
    }
    else {
      active = true;
      float  beta = 0.5 * (1. - dTmp) / (1. + dTmp);
      float gamma = (0.5 + beta) * Utils::highCos(theta).toFloat();
      b0 = (0.5 + beta - gamma) / 2.;
      b1 = 2. * b0;
      b2 = b0;
      a1 = -2. * gamma;
      a2 =  2. * beta;
    }
  }
}

  // if ((initialQ == 0.0f) || (initialFc < 1500) || (initialFc > 13500)) {
  //   active = false;
  // }
  // else {
  //   float omega, sinus, cosinus, alpha, a0;
 
  //   omega   = 2 * M_PI * (centsToFreq(initialFc) / config.samplingRate);
  //   sinus   = Utils::highSin(omega);
  //   cosinus = Utils::highCos(omega);
  //   alpha   = sinus / (2 * centibelToRatio(initialQ)); 

  //   a0 = 1 + alpha;
 
  //   a1 = (-2 * cosinus) / a0;
  //   a2 = ( 1 -   alpha) / a0;
  //   b0 = ( 1 - cosinus) / (2 * a0);
  //   b1 = ( 1 - cosinus) / a0;

  //   // b2 = ( 1 - cosinus) / (2 * a0);  <-- Similar to b0
  // }

  // x1 = x2 = y1 = y2 = 0.0f;

  // gain = 1.0f / sqrt(initialQ);
// }

void BiQuad::filter(sampleRecord & src, uint16_t length) 
{
  assert(length <= src.size());

  if (allActive && active) {
    for (int i = 0; i < length; i++) {

      // y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2]
      //                     - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]

      float val = (src[i].toFloat() * b0) + (b1 * x1) + (b2 * x2) - (a1 * y1) - (a2 * y2);

      x2 = x1; x1 = src[i].toFloat();
      y2 = y1; y1 =  val;

      src[i] = val * gain;
    }
  }
}

void BiQuad::showStatus(int spaces)
{
  using namespace std;

  cout << setw(spaces) << ' '
       << "BiQuad: "    << ((allActive && active) ? "Active" : "Inactive")
       << " [Fc:"       << initialFc 
       << ", Q:"        << initialQ
       << ", a1/a0:"    << a1
       << ", a2/a0:"    << a2 
       << ", b0_2/a0:"  << b0 
       << ", b1/a0:"    << b1 
       << ", b2/a0:"    << b2
       << ", gain:"     << gain
       << "]"
       << endl;
}