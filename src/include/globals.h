#ifndef _GLOBALS_
#define _GLOBALS_

#include <string>
#include <cassert>
#include "copyright.h"
#include <iostream>
#include <fstream>

#include <cstddef>
#include <new>

#include "new_handler_support.h"
#include "log.h"
#include "config.h"

#ifdef GLOBALS
# define PUBLIC
#else
# define PUBLIC extern
#endif

#if __ARM_FEATURE_DSP
  #define USE_NEON_INTRINSICS 1
#else
  #define USE_NEON_INTRINSICS 0
  typedef float float32_t;
#endif


#define samples24bits 1

class Mezzo;
class SoundFont2;
class Sound;
class Reverb;
class Equalizer;
class Poly;
class Midi;

#define MEZZO_VERSION  "MEZZO Version 1.0 - SF2 Sampling Synthesizer"

/// The Voice class manipulates buffers of samples
/// The Poly class retrieves buffers of samples from voices and build frames to
///    send back to the Sound output class

typedef float sample_t;    ///< A single sample
typedef sample_t * buffp;  ///< A pointer on a sample or a frame buffer, depending on context
typedef struct {
  sample_t left;
  sample_t right;
} frame_t;

#define KEY_NOT_USED 59999u

#define MAX_VOICES 512  ///< Maximum number of voices at any time

#define PRIVATE             static

#define BUFFER_FRAME_COUNT  256                               ///< Number of frame in a frame buffer
#define BUFFER_SAMPLE_COUNT BUFFER_FRAME_COUNT                ///< Number of samples in a sample buffer
#define SAMPLE_BLOCK_SIZE   (32 * 1024)                       ///< How many samples are loaded from the SF2 file each time

typedef std::array<sample_t, BUFFER_SAMPLE_COUNT> sampleRecord;
typedef std::array<frame_t,  BUFFER_FRAME_COUNT > frameRecord;

PUBLIC volatile bool keepRunning;

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

PUBLIC Log logger;

PUBLIC std::ofstream binFile;

#undef PUBLIC
#endif
