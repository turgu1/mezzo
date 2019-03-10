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

#ifndef _SAMPLE_
#define _SAMPLE_

#include "mezzo.h"

typedef class Sample * samplep;

class Synthesizer;

class Sample  : public NewHandlerSupport<Sample> {

private:
  #if loadInMemory
    buffp samples;
  #else
    #if samples24bits
      static int8_t  * data24;
      int8_t         * firstBlock24;
    #endif
    int16_t    * firstBlock;
    uint16_t     sizeFirstBlock;
  #endif

  static int16_t * data;     ///< Pointer on start of all samples data memory

  std::string  name;         ///< The name of the sample
  uint32_t     start;        ///< start offset in data
  uint32_t     end;          ///< end offset in data
  uint32_t     startLoop;
  uint32_t     endLoop;
  uint32_t     sampleRate;
  uint8_t      pitch;
  int8_t       correction;
  uint16_t     link;
  SFSampleLink linkType;
  uint32_t     sizeSample;
  uint32_t     sizeLoop;

  bool loaded;

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Sample(sfSample & info);
 ~Sample();

 /// Debugging method to show the current state of a sample
 void showStatus(int spaces);

  #if samples24bits
    static void setSamplesLocation(int16_t * dta, int8_t * dta24) { data = dta; data24 = dta24; };
  #else
    static void setSamplesLocation(int16_t * dta) { data = dta; };
  #endif

  bool load();

  /// This method returns data from the samples, managing the location
  /// (in the pre-loaded buffer or not) and the looping generation. The
  /// data is converted from its short int representation to a float
  /// with the value (-32768..32768) normalized to -1.0 .. 1.0
  /// Maximum size that can be loaded: 65535.

  uint16_t getData(sampleRecord & buff, uint32_t pos, Synthesizer & synth);
  buffp getData2(uint32_t & count, uint32_t pos, Synthesizer & synth);

  inline std::string &  getName() { return name;       }
  inline uint8_t       getPitch() { return pitch;      }
  inline uint32_t getSampleRate() { return sampleRate; }
  inline uint32_t      getStart() { return start;      }
  inline uint32_t        getEnd() { return end;        }
  inline uint32_t  getStartLoop() { return startLoop;  }
  inline uint32_t    getEndLoop() { return endLoop;    }
  inline int8_t   getCorrection() { return correction; }
};

#endif
