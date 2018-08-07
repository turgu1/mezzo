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

#ifndef _SYNTHESIZER_
#define _SYNTHESIZER_

#include <iostream>

#include "mezzo.h"

// A synthesizer is containing the generators and code to
// transform samples in relashionship with the generators. A synthesizer
// is attached to each voice and is central to the transformation of
// sounds in the way the soundfont designer wanted it to be ear.

class Synthesizer {

private:
  Vibrato   vib;
  Envelope  volEnvelope;
  BiQuad    biQuad;

  uint32_t  pos;
  uint32_t  start;
  uint32_t  end;
  uint32_t  startLoop;
  uint32_t  endLoop;
  uint32_t  sampleRate;
  uint32_t  sizeSample;
  uint32_t  sizeLoop;
  float     correctionFactor;
  float32_t left, right;
  int16_t   pan;
  int16_t   fineTune;
  uint8_t   rootKey;
  int8_t    keynum;
  int8_t    transpose;
  int8_t    velocity;
  bool      loop;
  bool      endOfSound;
  int16_t   lastValue;

  float attenuation;

  enum setGensType { set, adjust, init };
  void setGens(sfGenList * gens, uint8_t genCount, setGensType type);

  inline void toStereoAndAdd(frameRecord & dst, sampleRecord & src, uint16_t length) 
  {
    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0.0f;
        } while (length & 0x03);
      }
      assert(((length & 0x03) == 0) && (length >= 4) && (length <= BUFFER_SAMPLE_COUNT));
    #else
      assert((length >= 1) && (length <= BUFFER_SAMPLE_COUNT));
    #endif


    if (pan >=  250) {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;

        for (int i = 0; i < length; i += 4) {
          __builtin_prefetch(&dst[i].left);
          __builtin_prefetch(&src[i]);
          dstData = vld2q_f32(&dst[i].left);
          srcData = vld1q_f32(&src[i]);
          dstData.val[1] = vaddq_f32(dstData.val[1], srcData);
          vst2q_f32(&dst[i].left, dstData);
        }
      #else
        for (int i = 0; i < length; i++) dst[i].right += src[i];
      #endif
    }
    else if (pan <= -250) {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;

        for (int i = 0; i < length; i += 4) {
          __builtin_prefetch(&dst[i].left);
          __builtin_prefetch(&src[i]);
          dstData = vld2q_f32(&dst[i].left);
          srcData = vld1q_f32(&src[i]);
          dstData.val[0] = vaddq_f32(dstData.val[0], srcData);
          vst2q_f32(&dst[i].left, dstData);
        }
      #else
        for (int i = 0; i < length; i++) dst[i].left += src[i];
      #endif
    }
    else {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;

        for (int i = 0; i < length; i += 4) {
          __builtin_prefetch(&dst[i].left);
          __builtin_prefetch(&src[i]);
          dstData = vld2q_f32(&dst[i].left);
          srcData = vld1q_f32(&src[i]);

          dstData.val[0] = vmlaq_n_f32(dstData.val[0], srcData, left);
          dstData.val[1] = vmlaq_n_f32(dstData.val[1], srcData, right);

          vst2q_f32(&dst[i].left, dstData);
        }
      #else
        for (int i = 0; i < length; i++) {
          dst[i].left  += src[i] * left; 
          dst[i].right += src[i] * right;
        }
      #endif
    }
  }

  inline void setAttenuation  (int16_t a) { attenuation  = centibelToRatio(- (a >> 1)); }
  inline void addToAttenuation(int16_t a) { attenuation *= centibelToRatio(- (a >> 1)); }

public:

  inline void initGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, init);
  }
  inline void setGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, set);
  }
  inline void addGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, adjust);
  }
  void setDefaults(Sample * sample);

  void showStatus(int spaces);
  void completeParams(uint8_t note);

  inline uint32_t   getStart()       { return start;             }
  inline uint32_t   getEnd()         { return end;               }
  inline uint32_t   getStartLoop()   { return startLoop;         }
  inline uint32_t   getEndLoop()     { return endLoop;           }
  inline uint32_t   getSampleRate()  { return sampleRate;        }
  inline bool       isLooping()      { return loop;              }
  inline uint16_t   getPan()         { return pan;               }
  inline uint32_t   getSizeSample()  { return sizeSample;        }
  inline uint32_t   getSizeLoop()    { return sizeLoop;          }
  inline float      getCorrection()  { return correctionFactor;  }
  inline uint8_t    getRootKey()     { return rootKey;           }
  inline int8_t     getVelocity()    { return velocity;          }
  inline int8_t     getKeynum()      { return keynum;            }
  inline int8_t     getTranspose()   { return transpose;         }
  inline int16_t    getFineTune()    { return fineTune;          }
  inline Envelope * getVolEnvelope() { return &volEnvelope;      }

  inline void     setEndOfSound(bool val) { endOfSound = val;  }

  inline void     setLastValue(int16_t v) { lastValue = v;     }
  inline int16_t  getLastValue()          { return lastValue;  }

  /// Returns true if this call must be considered the end of the note (in the
  /// case where the envelope as been desactivated)
  inline bool keyHasBeenReleased() { return volEnvelope.keyHasBeenReleased(); }

  inline float vibrato(uint32_t pos) { return vib.nextValue(pos); }

  static bool areAllFilterActive()   { return BiQuad::areAllActive();   }
  static bool areAllVibratoActive()  { return Vibrato::areAllActive();  }
  static bool areAllEnvelopeActive() { return Envelope::areAllActive(); }

  static bool toggleFilter()     { return BiQuad::toggleAllActive();   }
  static bool toggleVibrato()    { return Vibrato::toggleAllActive();  }
  static bool toggleEnvelope()   { return Envelope::toggleAllActive(); }

  inline void applyEnvelopeAndGain(sampleRecord & src, uint16_t length, float gain) 
  {
    sampleRecord amps;

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0.0f;
        } while (length & 0x03);
      }
      assert(((length & 0x03) == 0) && (length >= 4) && (length <= BUFFER_SAMPLE_COUNT));
    #else
      assert((length >= 1) && (length <= BUFFER_SAMPLE_COUNT));
    #endif

    float32_t attGain = gain * attenuation;

    endOfSound = volEnvelope.getAmplitudes(amps, length);

    #if USE_NEON_INTRINSICS
      float32x4_t   srcData;
      float32x4_t   envData;

      for (int i = 0; i < length; i += 4) {
        __builtin_prefetch(&amps[i]);
        __builtin_prefetch(&src[i]);
        envData = vld1q_f32(&amps[i]);
        srcData = vld1q_f32(&src[i]);
        envData = vmulq_n_f32(envData, attGain);
        srcData = vmulq_f32(srcData, envData);
        vst1q_f32(&src[i], srcData);
      }
    #else
      for (uint16_t i = 0; i < length; i++) { 
        src[i] *= gain * amps[i]; 
      }
    #endif
  }

  inline bool transformAndAdd(frameRecord & dst, sampleRecord & src, uint16_t length)
  {
    biQuad.filter(src, length);

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0.0f;
        } while (length & 0x03);
      }
    #endif

    assert(length <= BUFFER_SAMPLE_COUNT);

    toStereoAndAdd(dst, src, length);

    pos += length;

    // if (endOfSound) std::cout << "End of Sound" << std::endl;
    // std::cout << "[" << amplVolEnv << "]" << std::endl;

    return endOfSound;
  }
};

#endif
