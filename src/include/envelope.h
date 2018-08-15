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

#ifndef _ENVELOPE_
#define _ENVELOPE_

#include "mezzo.h"

class Envelope
{
private:
  static bool allActive;

  enum State {
    START = 0, DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE, OFF
  };

  uint8_t  state;

  uint32_t delay;
  uint32_t attack;
  uint32_t hold;
  uint32_t decay;
  uint32_t release;

  int16_t  keynumToHold;
  int16_t  keynumToDecay;

  float    ratio;
  float    coef;
  float    base;
  uint32_t ticks;

  float amplitude;
  float sustain;

  volatile bool keyReleased;

public:

  Envelope() 
  { 
    ticks         = 
    delay         = 
    attack        = 
    hold          = 
    decay         = 0;

    keynumToHold  = 
    keynumToDecay = 0;

    ratio         = 
    base          =
    amplitude     = 0.0f;

    coef          =
    sustain       = 1.0f;
    
    release       = centsToSampleCount(-3600); 

    state         = START;
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool    areAllActive() { return allActive;              }

  inline void setDelay    (int16_t d) { delay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 5000, 0)); }
  inline void addToDelay  (int16_t d) { delay   *= (d == -32768) ? 1 :       centsToRatio(Utils::checkRange(d, -12000, 5000, 0)); }

  inline void setAttack   (int16_t a) { attack   = (a == -32768) ? 0 : centsToSampleCount(Utils::checkRange(a, -12000, 8000, 0)); }
  inline void addToAttack (int16_t a) { attack  *= (a == -32768) ? 1 :       centsToRatio(Utils::checkRange(a, -12000, 8000, 0)); }

  inline void setHold     (int16_t h) { hold     = (h == -32768) ? 0 : centsToSampleCount(Utils::checkRange(h, -12000, 5000, 0)); }
  inline void addToHold   (int16_t h) { hold    *= (h == -32768) ? 1 :       centsToRatio(Utils::checkRange(h, -12000, 5000, 0)); }

  inline void setDecay    (int16_t d) { decay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 8000, 0)); }
  inline void addToDecay  (int16_t d) { decay   *= (d == -32768) ? 1 :       centsToRatio(Utils::checkRange(d, -12000, 8000, 0)); }

  inline void setRelease  (int16_t r) { release  = (r == -32768) ? 0 : centsToSampleCount(Utils::checkRange(r, -12000, 8000, 0)); }
  inline void addToRelease(int16_t r) { release *= (r == -32768) ? 1 :       centsToRatio(Utils::checkRange(r, -12000, 8000, 0)); }

  inline void setKeynumToHold  (int16_t k) { keynumToHold  = k; }
  inline void addToKeynumToHold(int16_t k) { keynumToHold += k; }

  inline void setKeynumToDecay  (int16_t k) { keynumToDecay  = k; }
  inline void addToKeynumToDecay(int16_t k) { keynumToDecay += k; }

  // This is the decrease in level, expressed in centibels, to which the Volume 
  // Envelope value ramps during the decay phase. For the Volume Envelope, the 
  // sustain level is best expressed in centibels of attenuation from full scale. 
  // A value of 0 indicates the sustain level is full level; this implies a zero 
  // duration of decay phase regardless of decay time. A positive value indicates 
  // a decay to the corresponding level. Values less than zero are to be 
  // interpreted as zero; conventionally 1000 indicates full attenuation. 
  // For example, a sustain level which corresponds to an absolute value 12dB 
  // below of peak would be 120.

  inline void setSustain(int32_t s) 
  {
    if (s >= 1000) {
      sustain = 1.0f;
    }
    else if (s <= 0) {
      sustain = 0.0f;
    }
    else {
      sustain = centibelToRatio(- s);
    }
  }

  inline void addToSustain(int32_t s)
  { 
    if (s >= 1000) {
      sustain = 0.1f;
    }
    else if (s <= 0) {
    }
    else {
      sustain *= centibelToRatio(- s);
    }
  }

  inline float computeCoef(float ratio, uint32_t ticks)
  {
    return exp(-log((1.0f + ratio) / ratio) / ticks);
  }

  inline void setup(uint8_t note) 
  {
    (void) note;

    keyReleased   = false;
  }

  inline bool keyIsReleased() { return keyReleased; }

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope. A quick release means a shortened
  /// period to go to a 0 amplitude. If the envelope is inactive (as requested
  /// by the user)
  inline bool keyHasBeenReleased(bool quick = false) 
  {
    if (!allActive) return true; // This will fake the end of the sound

    release     = quick ? 8000 : release;
    keyReleased = true;

    return false;
  }

  inline void nextState()
  {
    switch (++state) {
      case DELAY:
        if (delay > 0) {
          ticks     = delay;
          amplitude = 0.0f;
          base      = 0.0f;
          coef      = 0.0f;
          break;
        }
        state     = ATTACK;

      case ATTACK:
        if (attack > 0) {
          amplitude = 0.0001f;
          ratio     = 0.03;
          ticks     = attack;
          coef      = computeCoef(ratio, ticks);
          base      = (1.0f + ratio) * (1.0f - coef);
          break;
        }
        state = HOLD;
        amplitude = 1.0f;

      case HOLD:
        if (hold > 0) {
          ticks     = hold;
          base      = 0.0f;
          coef      = 1.0f;
          break;
        }
        state = DECAY;

      case DECAY:
        if (decay > 0) {
          ticks     = decay;
          ratio     = 0.0001f;
          coef      = computeCoef(ratio, ticks);
          base      = (sustain - ratio) * (1.0f - coef);
          break;
        }
        state = SUSTAIN;
        amplitude = sustain;

      case SUSTAIN:
        ticks = 0xFFFFFFFFU;
        base      = 0;
        coef      = 1.0f;
        break;

      case RELEASE:
        break;

      default:
        state     = OFF;
        amplitude = 0.0f;
        base      = 0.0f;
        coef      = 0.0f;
        break;
    }
  }

  // From where we are in the stream of envelope levels, build an
  // array with the changes of amplitude to apply to a packet of
  // samples. Returns true if at the end of the envelope.
  //
  // If using NEON Intrinsics, length must be a multiple of 4.
  inline bool getAmplitudes(sampleRecord & amps, uint16_t length) 
  {
    if (!allActive) return false; // Fake this it is not the end of the sound

    if (state >= OFF) return true;

    if (keyReleased && (state < RELEASE)) {
      state = release == 0 ? OFF : RELEASE;
      ticks = release;
      ratio = 0.0001;
      coef  = computeCoef(ratio, ticks);
      base  = (- ratio) * (1.0f - coef);
      
      if (state >= OFF) return true;
    }

    #if USE_NEON_INTRINSICS

      assert(((length & 0x03) == 0) && (length >= 4));

      float bases[4];
      float coefs[4];
      static const float zero = 0.0f;
      static const float one  = 1.0f;

      coefs[0] = coef;
      coefs[1] = coef * coef;
      coefs[2] = coefs[1] * coef;
      coefs[3] = coefs[2] * coef;

      bases[0] = base;
      bases[1] = base + (base * coef);
      bases[2] = bases[1] + (base * coefs[1]);
      bases[3] = bases[2] + (base * coefs[2]);

      float32x4_t vcoefs = vld1q_f32(coefs);
      float32x4_t vbases = vld1q_f32(bases);

      float32x4_t vzeros = vld1q_dup_f32(&zero);
      float32x4_t vones  = vld1q_dup_f32(&one);

      for (int i = 0; i < length; i += 4) {

        float32x4_t vamplitudes = vld1q_dup_f32(&amplitude);
        float32x4_t vres        = vmlaq_f32(vbases, vamplitudes, vcoefs);
        vres                    = vminq_f32(vres, vones);
        vres                    = vmaxq_f32(vres, vzeros);

        vst1q_f32(&amps[i], vres);
        amplitude = amps[i + 3];

        if (ticks <= 4) {
          nextState();

          coefs[0] = coef;
          coefs[1] = coef     * coef;
          coefs[2] = coefs[1] * coef;
          coefs[3] = coefs[2] * coef;

          bases[0] = base;
          bases[1] = base + (base * coef);
          bases[2] = bases[1] + (base * coefs[1]);
          bases[3] = bases[2] + (base * coefs[2]);

          vcoefs = vld1q_f32(coefs);
          vbases = vld1q_f32(bases);
        }
        else {
          ticks -= 4;
        }
      }
    #else
      for (int i = 0; i < length; i++) {
        if (ticks-- == 0) nextState();
        amps[i] = amplitude = MAX(MIN(1.0f, base + amplitude * coef), 0.0f);
      }
    #endif


    return state == OFF;
  }

  void showStatus(int spaces) ;
};

#endif
