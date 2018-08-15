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

#ifndef _UTILS_
#define _UTILS_

#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

#define NOTE_FACTOR       16.3515978312874f

#define _12TH_ROOT_OF_2    1.0594630943593f
#define _1200TH_ROOT_OF_2  1.000577789506555f
#define _200TH_ROOT_OF_10  1.011579454259899f

#define centibelToRatio(x)    powf(_200TH_ROOT_OF_10, x)
#define centsToRatio(x)       powf(_1200TH_ROOT_OF_2, x)
#define centsToFreq(x)        (centsToRatio(x) * 8.176f)   // in Hz
#define centsToFreqRatio(x)   (1 / centsToFreq(x))         // in seconds
#define centsToSampleCount(x) (centsToRatio(x) * config.samplingRate)
#define noteFrequency(x)      (NOTE_FACTOR * powf(_12TH_ROOT_OF_2, x - 12))

class Utils {
public:
  inline static sampleRecord & shortToFloatNormalize(sampleRecord & dst, rawSampleRecord & src, int length)
  {
    const float32_t norm = 1.0 / 32768.0;

    assert(src != NULL);
    assert((length >= 1) && (length <= BUFFER_SAMPLE_COUNT));

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0;
        } while (length & 0x03);
      }
      assert((length >= 4) && (length <= BUFFER_SAMPLE_COUNT));

      for (int i = 0; i < length; i += 4) {
        __builtin_prefetch(&src[i]);
        int16x4_t   s16    = vld1_s16(&src[i]);
        int32x4_t   s32    = vmovl_s16(s16);
        float32x4_t f32    = vcvtq_f32_s32(s32);
        float32x4_t result = vmulq_n_f32(f32, norm);
        vst1q_f32(&dst[i], result);
      }
    #else
      for (int i = 0; i < length; i++) {
        dst[i] = src[i] * norm;
      }
    #endif
    return dst;
  }

  // This method clips values to be between -1.0 and 1.0 .
  // The destination buffer is the one supplied by the
  // Audio function (rtAudio). This to mitigate the need to copy
  // the content, as we use std::array internally in this
  // application, but rtAudio requires an C array of floats.
  static inline void clip(buffp dst, frameRecord & buff)
  {
    const float minusOne = -1.0f;
    const float one      =  1.0f;

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      float32x4_t minusOnes = vld1q_dup_f32(&minusOne);
      float32x4_t ones = vld1q_dup_f32(&one);

      for (uint16_t i = 0; i < buff.size(); i += 2) {
        float32x4_t data = vld1q_f32(&buff[i].left);
        data = vminq_f32(vmaxq_f32(data, minusOnes), ones);
        vst1q_f32(dst, data);
        dst += 4;
      }
    #else
      for (auto & element : buff) {
        *dst++  = MIN(MAX(element.left, minusOne), one);
        *dst++  = MIN(MAX(element.right, minusOne), one);
      }
    #endif
  }

  static bool fileExists(const char * name);

  // Read a line from the standard input. Returns true if
  // a 5 seconds timeout occurs. A line can be empty.
  static bool readStdIn(std::string & value);

  // Read a positive number from the standard input. Returns true if
  // a 5 seconds timeout occurs. If no number entered (empty line),
  // value will get -1.
  static bool getNumber(int16_t & value);

  inline static int16_t checkRange(int16_t value, int16_t min, int16_t max, int16_t defValue) {
    if ((value < min) || (value > max)) return defValue;
    return value;
  }

  // Trigo methods
  // From: http://lab.polygonal.de/2007/07/18/fast-and-accurate-sinecosine-approximation/

  // Low Precision sine/cosine
  // Error < 0.06

  // 1.27323954  = 4/pi
  // 0.405284735 =-4/(pi^2)

  inline static float lowSin(float x) {
    while (x < -3.14159265) x += 6.28318531;
    while (x >  3.14159265) x -= 6.28318531;

    if (x < 0) {
      return 1.27323954 * x + 0.405284735 * x * x;
    }
    else {
      return 1.27323954 * x - 0.405284735 * x * x;
    }
  }

  inline static float lowCos(float x) { return lowSin(x + 1.57079632); }

  // High Precision sine/cosine
  // (error < 0.0008)

  inline static float highSin(float x) {
    while (x < -3.14159265) x += 6.28318531;
    while (x >  3.14159265) x -= 6.28318531;

    if (x < 0) {
      float tmp = (1.27323954 * x) + (0.405284735 * x * x);

      if (tmp < 0) {
        return (.225 * ((tmp * -tmp) - tmp)) + tmp;
      }
      else {
        return (.225 * ((tmp *  tmp) - tmp)) + tmp;
      }
    }
    else {
      float tmp = 1.27323954 * x - 0.405284735 * x * x;

      if (tmp < 0)
        return (.225 * ((tmp * -tmp) - tmp)) + tmp;
      else
        return (.225 * ((tmp *  tmp) - tmp)) + tmp;
    }
  }

  inline static float highCos(float x) { return highSin(x + 1.57079632); }
};

#endif
