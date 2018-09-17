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
  inline static sampleRecord & rawToSampleRecord(sampleRecord & dst, rawSampleRecord & src, int length)
  {
    assert((length >= 1) && (length <= BUFFER_SAMPLE_COUNT));

    // Duration duration;

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0;
        } while (length & 0x03);
      }
      assert((length >= 4) && (length <= BUFFER_SAMPLE_COUNT));

      //int32x4_t  shift = vld1q_dup_s32(&nine);

      for (int i = 0; i < length; i += 4) {
        // __builtin_prefetch(&src[i]);
        int16x4_t   s16    = vld1_s16(&(src.data()[i]));
        int32x4_t   result = vshll_n_s16(s16, 9);
        vst1q_s32(&(dst.data()[i].v), result);
      }
    #else
      for (int i = 0; i < length; i++) {
        dst[i] = Fixed::normalize(src[i]);
      }
    #endif


    // long dur = duration.getElapse();

    // testMax = MAX(testMax, dur);
    // testMin = (testMin == -1) ? dur : MIN(testMin, dur);

    return dst;
  }

  // This method clips values to be between -1.0 and 1.0 .
  // The destination buffer is the one supplied by the
  // Audio function (portaudio). This to mitigate the need to copy
  // the content, as we use std::array internally in this
  // application, but portaudio requires an C array of floats.
  //
  // Clipping algorithm is faster using NEON
  static inline void clip(int16_t * dst, frameRecord & buff)
  {
    const Fixed minusOne = -0.999f;
    const Fixed one      =  0.99f;

    // Duration duration;

    #if USE_NEON_INTRINSICS_NEW
      int32x4_t minusOnes = vld1q_dup_s32(&minusOne.v);
      int32x4_t ones      = vld1q_dup_s32(&one.v);

      for (uint16_t i = 0; i < buff.size(); i += 2) {
        int32x4_t data   = vld1q_s32(&(buff.data()[i].left.v));
        int32x4_t data2  = vmaxq_s32(data, minusOnes);
                  data   = vminq_s32(data2, ones);
        int16x4_t result = vqshrn_n_s32(data, 9);
        vst1_s16(dst, result);
        dst += 4;
      }
    #else
      for (auto & element : buff) {
        *dst++  = element.left.max(minusOne).min(one).clip();
        *dst++  = element.right.max(minusOne).min(one).clip();
      }
    #endif

    // long dur = duration.getElapse();

    // testMax = MAX(testMax, dur);
    // testMin = (testMin == -1) ? dur : MIN(testMin, dur);
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
  // Error < ~0.06

  // 1.27323954  = 4/pi
  // 0.405284735 =-4/(pi^2)

  inline static Fixed lowSin(Fixed x) {
    while (x < Fixed(-3.14159265)) x += Fixed(6.28318531);
    while (x >  Fixed(3.14159265)) x -= Fixed(6.28318531);

    if (x < 0) {
      return Fixed(1.27323954) * x + Fixed(0.405284735) * x * x;
    }
    else {
      return Fixed(1.27323954) * x - Fixed(0.405284735) * x * x;
    }
  }

  inline static Fixed lowCos(Fixed x) { return lowSin(x + Fixed(1.57079632)); }

  // High Precision sine/cosine
  // (error < ~0.0008)

  inline static Fixed highSin(Fixed x) {
    while (x < Fixed(-3.14159265)) x += Fixed(6.28318531);
    while (x >  Fixed(3.14159265)) x -= Fixed(6.28318531);

    if (x < Fixed(0)) {
      Fixed tmp = (Fixed(1.27323954) * x) + (Fixed(0.405284735) * x * x);

      if (tmp < Fixed(0)) {
        return (Fixed(.225) * ((tmp * -tmp) - tmp)) + tmp;
      }
      else {
        return (Fixed(.225) * ((tmp *  tmp) - tmp)) + tmp;
      }
    }
    else {
      Fixed tmp = Fixed(1.27323954) * x - Fixed(0.405284735) * x * x;

      if (tmp < Fixed(0))
        return (Fixed(.225) * ((tmp * -tmp) - tmp)) + tmp;
      else
        return (Fixed(.225) * ((tmp *  tmp) - tmp)) + tmp;
    }
  }

  inline static Fixed highCos(Fixed x) { return highSin(x + Fixed(1.57079632)); }
};

#endif
