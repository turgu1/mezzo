#ifndef _GLOBALS_
#define _GLOBALS_

#include <string>
#include <cassert>
#include "copyright.h"

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

class Mezzo;
class SoundFont2;
class Sound;
class Reverb;
class Equalizer;
class Poly;

#define MEZZO_VERSION  "MEZZO Version 1.0 - SF2 Sampling Synthesizer"

typedef float sample_t;
typedef sample_t * buffp;

#define FRAME_SIZE          (2 * sizeof(sample_t))            ///< A frame contains left and righ samples
#define LOG_FRAME_SIZE      3                                 ///< Log in base 2 of the frame size (3 bits)
#define SAMPLING_RATE       44100                             ///< The usual sampling rate
#define BUFFER_FRAME_COUNT  128                               ///< Number of samples/frame in a sound buffer
#define SAMPLE_BLOCK_SIZE   (32 * 1024)

#define FRAME_BUFFER_SIZE   (BUFFER_FRAME_COUNT * FRAME_SIZE) ///< Size of a frame buffer in bytes
#define BUFFER_SAMPLE_COUNT (BUFFER_FRAME_COUNT * 2)          ///< Number of samples in a buffer

PUBLIC volatile bool keepRunning;
PUBLIC bool          interactive;
PUBLIC bool          silent;

PUBLIC Mezzo      * mezzo;
PUBLIC SoundFont2 * soundFont;
PUBLIC Sound      * sound;
PUBLIC Equalizer  * equalizer;
PUBLIC Reverb     * reverb;
PUBLIC Poly       * poly;
PUBLIC Log logger;

#undef PUBLIC
#endif