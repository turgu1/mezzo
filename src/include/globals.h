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

#ifndef _GLOBALS_
#define _GLOBALS_

#include <string>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstddef>
#include <new>

#include <atomic>

#include "log.h"
#include "config.h"

#ifdef GLOBALS
# define PUBLIC
#else
# define PUBLIC extern
#endif

// All DSP related instructions are made using NEON intrinsics as RASPBERRY PI 2/3 are targetted.
// Compiling on an Intel x86 processor is possible using the translation from NEON to SSE probivided
// by the neon_2_sse.h include file. See https://github.com/intel/ARM_NEON_2_x86_SSE

#if 0
#if __ARM_FEATURE_DSP
  #define USE_NEON_INTRINSICS 1
  #include <arm_neon.h>
#else
  #define USE_NEON_INTRINSICS 1
  #include <neon_2_sse.h>
#endif
#endif

// if you ever wants to not used DSP intrinsics, the float32_t definition is required

#if !USE_NEON_INTRINSICS
  typedef float float32_t;
#endif

// Todo: Complete 24 bits support
#define samples24bits 0

class Mezzo;
class SoundFont2;
class Sound;
class Reverb;
class Equalizer;
class Poly;
class Midi;
class Metronome;

#define MEZZO_VERSION  "MEZZO Version 1.0 - SF2 Sampling Synthesizer"

/// The Voice class manipulates buffers of samples
/// The Poly class retrieves buffers of samples from voices and build frames to
///    send back to the Sound output class

typedef Fixed sample_t;     ///< A single sample
typedef sample_t * buffp;   ///< A pointer on a sample or a frame buffer, depending on context
typedef struct {
  sample_t left;
  sample_t right;
} frame_t;

#define KEY_NOT_USED 59999u

#define MAX_VOICES 620      ///< Maximum number of voices at any time

#define PRIVATE             static

#define BUFFER_FRAME_COUNT  256                               ///< Number of frame in a frame buffer
#define BUFFER_SAMPLE_COUNT BUFFER_FRAME_COUNT                ///< Number of samples in a sample buffer
#define SAMPLE_BLOCK_SIZE   (32 * 1024)                       ///< How many samples are loaded from the SF2 file each time

typedef std::array<sample_t, BUFFER_SAMPLE_COUNT> sampleRecord;
typedef std::array<frame_t,  BUFFER_FRAME_COUNT > frameRecord;
typedef std::array<int16_t,  BUFFER_SAMPLE_COUNT> rawSampleRecord;

PUBLIC std::atomic<bool> keepRunning;

// Statistics

PUBLIC int      maxVoicesMixed;     ///< Maximum number of voices that have been mixed simultaneously
PUBLIC long     mixerDuration;      ///< Maximum duration of the mixer function during play (nanoseconds)
PUBLIC long     reverbMinDuration;  ///< Minimum duration of the reverb process
PUBLIC long     reverbMaxDuration;  ///< Maximim duration of the reverb process
PUBLIC sample_t maxVolume;          ///< Maximum gain used un mixing voices

PUBLIC Mezzo      * mezzo;
PUBLIC SoundFont2 * soundFont;
PUBLIC Sound      * sound;
PUBLIC Equalizer  * equalizer;
PUBLIC Reverb     * reverb;
PUBLIC Poly       * poly;
PUBLIC Midi       * midi;
PUBLIC Metronome  * metronome;

PUBLIC Log logger;

PUBLIC std::ofstream binFile;

#undef PUBLIC
#endif
