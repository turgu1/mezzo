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

#define USE_NEON_INTRINSICS 0

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
typedef sample_t * buffp;  ///< A pointer on a sample or a frame buffer

#define KEY_NOT_USED 59999u

#define PRIVATE             static

#define FRAME_SIZE          (2 * sizeof(sample_t))            ///< A frame contains left and righ samples
#define SAMPLE_SIZE         (sizeof(sample_t))                ///< A sample contains a float
#define LOG_FRAME_SIZE      3                                 ///< Log in base 2 of the frame size (3 bits)
#define LOG_SAMPLE_SIZE     2                                 ///< Log in base 2 of the sample size (2 bits)
#define BUFFER_FRAME_COUNT  256                               ///< Number of samples/frame in a sound buffer
#define SAMPLE_BLOCK_SIZE   (32 * 1024)                       ///< How many samples are loaded from the SF2 file each time

#define SAMPLE_BUFFER_SIZE         (BUFFER_FRAME_COUNT * SAMPLE_SIZE)///< Size of a sample buuffer in bytes
#define FRAME_BUFFER_SIZE          (BUFFER_FRAME_COUNT * FRAME_SIZE) ///< Size of a frame buffer in bytes
#define FRAME_BUFFER_SAMPLE_COUNT  (BUFFER_FRAME_COUNT * 2)          ///< Number of samples in a buffer
#define SAMPLE_BUFFER_SAMPLE_COUNT (BUFFER_FRAME_COUNT)              ///< Number of samples in a buffer

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
