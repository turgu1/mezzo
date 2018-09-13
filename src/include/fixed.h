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

#ifndef _FIXED_
#define _FIXED_

#include <cassert>
#include <cstdint>

// Fixed point arithmetic

// 0100 1010.1010 0010 1101 0101 0101 0110

class Fixed
{
private:
  static const int32_t scale        = 24;
  static const int32_t epsilon      =  1;
  static const int32_t fractionMask = 0xFFFFFFFF >> (32 - scale);
  static const int32_t wholeMask    = -1 ^ fractionMask;

  static const int32_t minValue     = 0x80000000;
  static const int32_t maxValue     = 0x7FFFFFFF;

  int32_t v;

public:
  Fixed() : v(0) { }
  Fixed(const Fixed & b) { v = b.v; }
  Fixed(const int i, const int f) : v((i << scale) + f) { assert((i >= -128   ) && (i <= 127         ));
                                                          assert((f >=    0   ) && (f <= fractionMask)); }
  Fixed(int32_t x) : v(x << scale)                      { assert((x >= -128   ) && (x <= 127         )); }
  Fixed(float   x) : v(x * (float)(1 << scale))         { assert((x >= -128.0f) && (x <= 127.9999f   )); }
  Fixed(double  x) : v(x * (double)(1 << scale))        { assert((x >= -128.0 ) && (x <= 127.9999    )); }

  inline int32_t   toInt() const { return v >> scale;                         }
  inline float   toFloat() const { return (float )(v) / (float )(1 << scale); }
  inline double toDouble() const { return (double)(v) / (double)(1 << scale); }

  inline const Fixed   fraction () const { Fixed a(*this); a.v &= fractionMask; return a; }
  inline const Fixed   whole    () const { Fixed a(*this); a.v &= wholeMask;    return a; }
  
  inline const Fixed & min      (const Fixed & b) const { return (*this <= b) ? *this : b; }
  inline const Fixed & max      (const Fixed & b) const { return (*this >= b) ? *this : b; }

  template <typename T> inline       Fixed & operator+=(const T b) { v += Fixed(b).v; return *this;   }
  template <typename T> inline       Fixed & operator-=(const T b) { v -= Fixed(b).v; return *this;   }
  template <typename T> inline       Fixed & operator*=(const T b) { v = ((int64_t) v * (int64_t)(Fixed(b).v)) >> scale;   return *this; }
  template <typename T> inline       Fixed & operator/=(const T b) { v = (((int64_t) v) << scale) / (int64_t)(Fixed(b).v); return *this; }

  template <typename T> inline const Fixed   operator+ (const T b) const { return Fixed(*this) += b; }
  template <typename T> inline const Fixed   operator- (const T b) const { return Fixed(*this) -= b; }
  template <typename T> inline const Fixed   operator* (const T b) const { return Fixed(*this) *= b; }
  template <typename T> inline const Fixed   operator/ (const T b) const { return Fixed(*this) /= b; }

  inline const Fixed operator-() { Fixed f(*this); f.v = -f.v; return f; }
  
  template <typename T> inline       Fixed & operator= (const T b)       { v = Fixed(b).v; return *this;    }

  template <typename T> inline bool    operator==(const T b) const { return v == Fixed(b).v; }
  template <typename T> inline bool    operator!=(const T b) const { return v != Fixed(b).v; }
  template <typename T> inline bool    operator< (const T b) const { return v <  Fixed(b).v; }
  template <typename T> inline bool    operator> (const T b) const { return v >  Fixed(b).v; }
  template <typename T> inline bool    operator<=(const T b) const { return v <= Fixed(b).v; }
  template <typename T> inline bool    operator>=(const T b) const { return v >= Fixed(b).v; }

  inline static const Fixed minVal() { return Fixed(-128, 0); }
  inline static const Fixed maxVal() { return Fixed( 127, fractionMask); }
};

#endif
