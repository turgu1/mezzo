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

#ifndef _REVERB_
#define _REVERB_

// This module implements the FreeVerb reverb algorithm with optimization for
//  ARM NEON Vector instructions
//
// A C++ version of  FreeVerb algorithm is available at the following link:
//
//    https://github.com/gburlet/FreeVerb
//
// A good documentation on the FreeVerb algorithm is available at the following link.
// This implementation is based on this documentation:
//
//    https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
//
//

#if USE_NEON_INTRINSICS
  // PADDING is extra space used at the end of each history vectors to simplify
  // loading data in NEON vectors.
  #define PADDING 4
#else
  #define PADDING 0
#endif

#define REVERB_COMB_COUNT 8
#define REVERB_AP_COUNT   4

class Reverb : public NewHandlerSupport<Reverb> {

 private:

  static const int comb_m[REVERB_COMB_COUNT];
  static const int   ap_m[REVERB_AP_COUNT];

  // The following structures implement FIFO (First In, First Out) to
  // compile Z^-n values in use by the reverb algorithm, supplying delayed
  // access to fitered data from previous algorithm execution

  struct fifo_struct {
    buffp buff;
    buffp head;
    buffp tail;
    buffp end;
  };

  typedef struct fifo_struct fifo_t;
  typedef fifo_t * fifop;

  #if USE_NEON_INTRINSICS

    // Those are the ARM NEON SIMD vectorized version of PUT and GET.
    // PUTv insure that if data is put after the end of the buffer space,
    // it is copied back to the beginning of the array. The redundancy of
    // data insure proper working conditions for vector based instructions
    // to quickly get access to 4 data entries in a row.
    //
    // Here is the non-optimized data copy version:
    //
    /*
      #define PUTv(ptr,v) vst1q_f32(ptr->tail, v);                 \
                          if (ptr->tail == ptr->buff) vst1q_f32(ptr->end, v); \
                          if ((ptr->tail += 4) >= ptr->end) {    \
                            int k = ptr->tail - ptr->end;         \
                            if (k > 0) memcpy(ptr->buff, ptr->tail - k, k << 2); \
                            ptr->tail = ptr->buff + k;         \
                          }
    */
    // Here is the optimized version (memcpy replaced with SIMD instructions):

    inline void PUTv(fifop ptr, float32x4_t v) {
      vst1q_f32(ptr->tail, v);
      if (ptr->tail == ptr->buff) vst1q_f32(ptr->end, v);
      if ((ptr->tail += 4) >= ptr->end) {
        int k = ptr->tail - ptr->end;
        if (k > 0)
          vst1q_f32((ptr)->buff,
                    k == 1 ? vextq_f32(vld1q_f32(ptr->tail - 4),
                                       vld1q_f32(ptr->buff + 1),
                                       3) :
                    k == 2 ? vextq_f32(vld1q_f32(ptr->tail - 4),
                                       vld1q_f32(ptr->buff + 2),
                                       2) :
                    vextq_f32(vld1q_f32(ptr->tail - 4),
                              vld1q_f32(ptr->buff + 3),
                              1));
        ptr->tail = ptr->buff + k;
      }
    }

    inline void GETv(fifop ptr, float32x4_t &v) {
      v = vld1q_f32(ptr->head);
      if ((ptr->head += 4) >= ptr->end)
        ptr->head = ptr->buff + (ptr->head - ptr->end);
    }

  #else

    // The following inline methods are used to get and put values in FIFO arrays
    // in used with the reverb algorithm. These arrays are strored inside ap_sruct
    // and comb_struct.

    inline void PUT(fifop ptr, float v) {
      *(ptr->tail++) = v;
      if (ptr->tail >= ptr->end) ptr->tail = ptr->buff;
    }

    inline void GET(fifop ptr, float &v) {
      v = *(ptr->head++);
      if (ptr->head >= ptr->end) ptr->head = ptr->buff;
    }

  #endif

  fifo_t  leftCombs[REVERB_COMB_COUNT];
  fifo_t rightCombs[REVERB_COMB_COUNT];
  fifo_t     leftAp[REVERB_AP_COUNT];
  fifo_t    rightAp[REVERB_AP_COUNT];

  float    leftLast[REVERB_COMB_COUNT];
  float   rightLast[REVERB_COMB_COUNT];

  float dryWet;    // proportion of dry vs wet mix (0 .. 1.0)
  float roomSize;  // 0 .. 1.0 Nothing usefull below 7.0
  float damping;   // 0 .. 1.0
  float width;     // 0 .. 1.0
  float apGain;    // 0 .. 1.0

  static void outOfMemory();
  void adjustValue(char ch);

 public:
   Reverb();
  ~Reverb();

  inline void setRoomSize(float value) { roomSize = value; }
  inline void setDamping(float value)  { damping = value;  }
  inline void setWidth(float value)    { width = value;    }
  inline void setDryWet(float value)   { dryWet = value;   }

  void process(frameRecord & buff);
  void interactiveAdjust();
};

#endif
