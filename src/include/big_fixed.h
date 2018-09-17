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

#ifndef _BIG_FIXED_
#define _BIG_FIXED_

#include <cassert>
#include <cstdint>

// Fixed point arithmetic

// 0100 1010.1010 0010 1101 0101 0101 0110

class BigFixed
{
private:
  static const int64_t scale        = 24;
  static const int64_t epsilon      =  1;
  static const int64_t fractionMask = 0xFFFFFFFFFFFFFFFFll >> (64 - scale);
  static const int64_t wholeMask    = -1 ^ fractionMask;

  static const int64_t minValue     = 0x8000000000000000ll;
  static const int64_t maxValue     = 0x7FFFFFFFFFFFFFFFll;

  static const int64_t minInt       = 0x8000000000ll;
  static const int64_t maxInt       = 0x7FFFFFFFFFll;

public:
  int64_t v;
  BigFixed() : v(0) { }
  BigFixed(const BigFixed & b) { v = b.v; }
  BigFixed(const int64_t i, const int f) : v((i << scale) + f) { assert((i >= minInt   ) && (i <= maxInt         ));
                                                              assert((f >=      0   ) && (f <= fractionMask      )); }
  BigFixed(int64_t x) : v(x << scale)                      { assert((x >= minInt   ) && (x <= maxInt         )); }
  BigFixed(float   x) : v(x * (float)(1 << scale))         { }//assert((x >= -128.0f) && (x <= 127.9999f   )); }
  BigFixed(double  x) : v(x * (double)(1 << scale))        { }//assert((x >= -128.0 ) && (x <= 127.9999    )); }
  BigFixed(Fixed b) : v(b.v) {}

  inline int64_t   toInt() const { return v >> scale;                         }
  inline float   toFloat() const { return (float )(v) / (float )(1 << scale); }
  inline double toDouble() const { return (double)(v) / (double)(1 << scale); }

  inline const BigFixed   fraction () const { BigFixed a(*this); a.v &= fractionMask; return a; }
  inline const BigFixed   whole    () const { BigFixed a(*this); a.v &= wholeMask;    return a; }
  
  inline const BigFixed & min      (const BigFixed & b) const { return (*this <= b) ? *this : b; }
  inline const BigFixed & max      (const BigFixed & b) const { return (*this >= b) ? *this : b; }

  template <typename T> inline       BigFixed & operator+=(const T b) { v += BigFixed(b).v; return *this;   }
  template <typename T> inline       BigFixed & operator-=(const T b) { v -= BigFixed(b).v; return *this;   }
  template <typename T> inline       BigFixed & operator*=(const T b) { v = ((int64_t) v * (int64_t)(BigFixed(b).v)) >> scale;   return *this; }
  template <typename T> inline       BigFixed & operator/=(const T b) { v = (((int64_t) v) << scale) / (int64_t)(BigFixed(b).v); return *this; }

  template <typename T> inline const BigFixed   operator+ (const T b) const { return BigFixed(*this) += b; }
  template <typename T> inline const BigFixed   operator- (const T b) const { return BigFixed(*this) -= b; }
  template <typename T> inline const BigFixed   operator* (const T b) const { return BigFixed(*this) *= b; }
  template <typename T> inline const BigFixed   operator/ (const T b) const { return BigFixed(*this) /= b; }

  inline const BigFixed operator-() { BigFixed f(*this); f.v = -f.v; return f; }
  
  template <typename T> inline       BigFixed & operator= (const T b)       { v = BigFixed(b).v; return *this;    }

  template <typename T> inline bool    operator==(const T b) const { return v == BigFixed(b).v; }
  template <typename T> inline bool    operator!=(const T b) const { return v != BigFixed(b).v; }
  template <typename T> inline bool    operator< (const T b) const { return v <  BigFixed(b).v; }
  template <typename T> inline bool    operator> (const T b) const { return v >  BigFixed(b).v; }
  template <typename T> inline bool    operator<=(const T b) const { return v <= BigFixed(b).v; }
  template <typename T> inline bool    operator>=(const T b) const { return v >= BigFixed(b).v; }

  inline static const BigFixed minVal() { BigFixed a; a.v = minValue; return a; }
  inline static const BigFixed maxVal() { BigFixed a; a.v = maxValue; return a; }
};

#endif
