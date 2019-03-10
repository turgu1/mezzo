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

#include <cmath>
#include <iomanip>

#include "mezzo.h"

#define BIG_NUMBERS 0

// In midi, there is a potential of 128 note values
// The vector will get scale factors for offsets between two notes
// ranging  from -127 to +127
#define SCALE_FACTOR_COUNT  ((127 * 2) + 1)

/// This vector contains the scale factors required to modify the pitch of a note to obtain a targeted
/// note sound. Please look in method Voice::Voice() for initialization values.

PRIVATE bool  scaleFactorsInitialized = false;
PRIVATE double scaleFactors[SCALE_FACTOR_COUNT];

bool     Voice::showPlayingState = false;
uint32_t Voice::nextSeq = 0;

double Voice::getScaleFactor(int16_t diff)
{
  return scaleFactors[diff + 127];
}

#if !loadInMemory
  void Voice::feedFifo()
  {
    if (isActive() && isAlive()) {
      if (!fifo.isFull()) {
        if (__sync_lock_test_and_set(&stateLock, 1) == 0) {
          uint16_t count = sample->getData(
            fifo.getTail(),
            fifoLoadPos,
            synth
          );
          if (count) {
            fifoLoadPos += count;
            fifo.setSampleCount(count);
            fifo.push();
          }
          else {
            //inactivate();
          }
          END();
        }
      }
    }
  }

  void Voice::prepareFifo()
  {
    fifo.clear();
    for (int i = 0; i < 4; i++)
    {
      uint16_t count = sample->getData(
        fifo.getTail(),
        fifoLoadPos,
        synth
      );
      if (count) {
        fifoLoadPos += count;
        fifo.setSampleCount(count);
        fifo.push();
      }
      else {
        return;
      }
    }
  }
#endif

//---- Voice() ----

Voice::Voice()
{
  active         = false;
  state          = DORMANT;
  stateLock      = 0;
  sample         = NULL;
  noteIsOn       = false;
  keyIsOn        = false;
  next           = NULL;

  if (!scaleFactorsInitialized) {
    scaleFactorsInitialized = true;

    // As per "Twelve-tone equal temperament" described at this url:
    //   https://en.wikipedia.org/wiki/Equal_temperament

    for (int i = 0; i < SCALE_FACTOR_COUNT; i++) {
      scaleFactors[i] = pow(2, (i - 127) / 12.0);
    }
  }
}

//---- ~Voice() ----

Voice::~Voice()
{
}

//---- setup() ----
//
// This method will connect the voice object with the sample and the preset.
// It will then initialize the voice, load the fifo, prepare the synthesizer
// and get ready to sound the sample. It is called when the user had struck
// a key and then require a sound to be played.

void Voice::setup(samplep      _sample,
                  uint8_t      _note,
                  float        _gain,
                  Synthesizer  _synth,
                  Preset     & _preset,
                  uint16_t     _presetZoneIdx)
{
  // Connect the sample with the voice
  sample         = _sample;
  synth          = _synth;
  gain           = _gain;

  note           =  (synth.getKeynum() == -1) ? _note : synth.getKeynum();

  outputPos      =     0;
  sampleBuffPos  =     0;
  sampleBuffSize =     0;

  noteIsOn       =  true;
  keyIsOn        =  true;
  active         = false;
  bufferReady    = false;

  // Feed something in the Fifo ring buffer before activation
  #if !loadInMemory
    fifoLoadPos    =     0;
    prepareFifo();
  #endif

  synth.addGens(_preset.getGlobalGens(), _preset.getGlobalGenCount());
  synth.addGens(_preset.getZoneGens(_presetZoneIdx), _preset.getZoneGenCount(_presetZoneIdx));

  synth.completeParams(note);

  factor = scaleFactors[(note - synth.getRootKey() + synth.getTranspose()) + 127] * synth.getCorrection();

  if (sample->getSampleRate() != config.samplingRate) {
    // resampling is required
    factor *= (((float)sample->getSampleRate()) / ((float)config.samplingRate));
  }

  sampleBuffSize  =  0;

  #if !loadInMemory
    // This initialization is required as the first loop in feedBuffer will retrieve the last
    // 4 positions in the buffer and put them at the beginning to simplify the algorithm

    sampleBuff[0] =
    sampleBuff[1] =
    sampleBuff[2] =
    sampleBuff[3] = 0.0f;
  #endif

  seq = nextSeq++;

  // Retrieve the first packet. The others will be fed through a thread after activation.

  feedBuffer(true);

  BEGIN();
    activate();     // Must be the last flag set. The threads are watching it...
  END();

  if (showPlayingState) showStatus(2);
}

#if !loadInMemory
  int Voice::retrieveFifoSamples(scaleRecord & buff, int pos)
  {
    int readSampleCount;

    if (fifo.isEmpty()) {
      // No more data available or the thread was not fast enough to get data on time
      //if (isActive()) std::cout << "FIFO Empty!!" << std::endl;
      readSampleCount = 0;
    }
    else {
      sampleRecord & src = fifo.getHead();
      readSampleCount = fifo.getSampleCount();
      std::copy(std::begin(src), std::end(src), &buff[pos]);
      fifo.pop();
    }

    return readSampleCount;
  }
#endif

sampleRecord & Voice::getBuffer(int16_t * count)
{
  *count = bufferReady ? bufferSize : -1;
  return buffer;
}

void Voice::feedBuffer(bool bypass)
{
  float    temp;
  float    fractionalPart;
  #ifndef NDEBUG
    uint32_t oldIntegralPart = 0;
  #endif
  uint32_t integralPart    = 0;

  if (bypass || (isActive() && !bufferReady)) {

    int count;

    // outputPos is the position where we are in the output as a number
    // of samples since the start of the note. samplePos is where we need to
    // get something from the sample, taking into account pitch changes, resampling
    // and modulation of all kind. buffIndex is the specific index in the
    // retrieved buffer.

    double lastSamplePos = ((float) outputPos) * factor;

    // Loop to completely fill the buffer with scaled samples
    for (count = 0; count < BUFFER_SAMPLE_COUNT; count++) {

      // The vibrato gives a bias to the sample position to be acquired
      double samplePos = lastSamplePos + synth.vibrato(outputPos);

      lastSamplePos += factor; // prepare for next loop

      // The following is working as samplePos is a positive number...

      fractionalPart = modf(samplePos, &temp);
      #ifndef NDEBUG
        oldIntegralPart = integralPart;
      #endif
      integralPart   = temp;

      assert(fractionalPart >= 0);
      assert(oldIntegralPart <= integralPart);

      #if loadInMemory
        while (integralPart >= (sampleBuffPos + sampleBuffSize)) {
          sampleBuffPos += sampleBuffSize;
          sampleBuff = sample->getData2(sampleBuffSize, sampleBuffPos, synth);
          //std::cout << count << " / " << integralPart << " / " << sampleBuffPos << " / " << sampleBuffSize << std::endl << std::flush;
          if (sampleBuffSize == 0) break;
        }

        if (sampleBuffSize == 0) break;

        assert(sampleBuff != NULL);
        assert(integralPart >= sampleBuffPos);

        uint32_t buffIndex = integralPart - sampleBuffPos;

        assert(buffIndex < sampleBuffSize);

        float * y = &sampleBuff[buffIndex];
      #else

        int16_t buffIndex = (integralPart % BUFFER_SAMPLE_COUNT) - 2;

        if (integralPart >= (sampleBuffPos + sampleBuffSize)) {
          sampleBuffPos += sampleBuffSize;

          // Retrieve the last 4 samples from the end of the buffer and put them at
          // the beginning to ensure proper interpolation at the beginning of next
          // buffer processing.
          //
          // For the first samples retrieval at the beginning of a note to be played,
          // the last 4 samples have been initialized to zero (0.0f) by the setup()
          // method.

          std::copy(&sampleBuff[sampleBuffSize], &sampleBuff[sampleBuffSize + 4], &sampleBuff[0]);

          if ((sampleBuffSize = retrieveFifoSamples(sampleBuff, 4)) == 0) {
            break;
          }
          assert((!synth.isLooping()) || (sampleBuffSize == BUFFER_SAMPLE_COUNT));
        }

        assert(buffIndex >= -2);

        float * y = &sampleBuff[buffIndex + 4];
      #endif

      buffer[count] = y[0] + ((y[1] - y[0]) * fractionalPart);

      outputPos++;
    }

    if (count > 0) {
      synth.applyEnvelopeAndGain(buffer, count, gain * config.masterVolume);
    }

    bufferSize = count;
    bufferReady = true;
  }
}

void Voice::showStatus(int spaces)
{
  using namespace std;

  PRIVATE const char *  stateStr[4] = { "DORMANT", "ALIVE" };

  cout
       << setw(spaces) << ' '
       << "Voice: "   << (active ? "Active" : "Inactive")
       << " [state:"  << (stateStr[state])
       << " pos:"     << (outputPos)
       << " sample:"  << (sample == NULL ? "none" : "see below")
       << " resampling factor:" << factor
       << " note:"    << (+note)
       << " gain:"    << (gain)
       << " sbpos:"   << (sampleBuffPos)
       << "]" << endl;

  if (sample != NULL) sample->showStatus(4 + spaces);

  synth.showStatus(4 + spaces);
}

#if 0
#if 1

/// Lagrange 4th order interpolation polynomes.
///
/// We used four consecutive locations (named x1 to x4) and compute
/// the following equations using x1 = -1, x2 = 0, x3 = 1, x4 = 2:
///
///      (x  - x2)(x  - x3)(x  - x4)
/// P1 = --------------------------- = x³ - 3x² + 2x / -6 = x ((3 - x) x - 2) / 6
///      (x1 - x2)(x1 - x3)(x1 - x4)
///
///      (x  - x1)(x  - x3)(x  - x4)
/// P2 = --------------------------- = x³ - 2x² - x + 2 / 2 = x ((x - 2) x - 1) + 2 / 2
///      (x2 - x1)(x2 - x3)(x2 - x4)
///
///
///      (x  - x1)(x  - x2)(x  - x4)
/// P3 = --------------------------- = x³ - x² - 2x / -2 = x ((1 - x) x + 2) / 2
///      (x3 - x1)(x3 - x2)(x3 - x4)
///
///      (x  - x1)(x  - x2)(x  - x3)
/// P4 = --------------------------- = x³ - x / 6 = x (x² - 1) / 6
///      (x4 - x1)(x4 - x3)(x4 - x3)
///
/// (the last equation on the right are Horner's forms that optimize
///  the number of arithmetic operations)
///
/// These equations are used in a "sliding window" for wich we only
/// compute interpolation located between x2 and x3.

#if BIG_NUMBERS
  inline double P1(double x) { return ((x * ((3.0L - x) * x - 2.0L)) / 6.0L); }
  inline double P2(double x) { return ((x * ((x - 2.0L) * x - 1.0L) + 2.0L) / 2.0L); }
  inline double P3(double x) { return ((x * ((1.0L - x) * x + 2.0L)) / 2.0L); }
  inline double P4(double x) { return ((x * ((x * x) - 1.0L)) / 6.0L); }
#else
  inline float P1(float x) { return ((x * ((3.0f - x) * x - 2.0f)) / 6.0f); }
  inline float P2(float x) { return ((x * ((x - 2.0f) * x - 1.0f) + 2.0f) / 2.0f); }
  inline float P3(float x) { return ((x * ((1.0f - x) * x + 2.0f)) / 2.0f); }
  inline float P4(float x) { return ((x * ((x * x) - 1.0f)) / 6.0f); }
#endif

int Voice::getSamples(buffp buff, int length)
{
  int count = 0;

  assert(sampleBuff != NULL);
  assert(buff != NULL);
  assert(length > 0);

  // outputPos is the postion where we are in the output as a number
  // of samples since the start of the note. samplePos is where we need to
  // get someting from the sample, taking into account pitch changes, resampling
  // and modulation of all kind. buffIndex is the specific index in the
  // retrieved buffer.

  #if BIG_NUMBERS
    double lastSamplePos = ((float) outputPos) * factor;
  #else
    float lastSamplePos = ((float) outputPos) * factor;
  #endif

  while (length--) {

    #if BIG_NUMBERS
      double samplePos = lastSamplePos + synth.vibrato(outputPos);
    #else
      float samplePos = lastSamplePos + synth.vibrato(outputPos);
    #endif

    lastSamplePos += factor;

    // The following is working as samplePos is a positive number...

    uint32_t integralPart   = samplePos;
    #if BIG_NUMBERS
      double fractionalPart = samplePos - integralPart;
    #else
      float  fractionalPart = samplePos - integralPart;
    #endif

    int16_t buffIndex = (integralPart % BUFFER_SAMPLE_COUNT) - 2;

    if (integralPart >= (sampleBuffPos + sampleBuffSize)) {
      sampleBuffPos += sampleBuffSize;

      // Retrieve the last 4 samples from the end of the buffer and put them at
      // the beginning to ensure proper interpolation at the beginning of next
      // buffer.
      //
      // For the first samples retrieval at the beginning of a note to be played,
      // the last 4 samples have been initialized to zero (0.0f) by the setup()
      // method.
      memcpy(sampleBuff, &sampleBuff[sampleBuffSize], 4 << LOG_SAMPLE_SIZE);

      if ((sampleBuffSize = retrieveFifoSamples(sampleBuff, 4)) == 0) break;
      assert((!synth.isLooping()) || (sampleBuffSize == BUFFER_SAMPLE_COUNT));
    }

    assert(buffIndex >= -2);

    float * y = &sampleBuff[buffIndex - 1 + 4];

    *buff++ = (P1(fractionalPart) * y[0]) +
              (P2(fractionalPart) * y[1]) +
              (P3(fractionalPart) * y[2]) +
              (P4(fractionalPart) * y[3]);

    outputPos++;
    count++;
  }

  return count;
}

#undef P1
#undef P2
#undef P3
#undef P4

#else

/// Lagrange 7th order interpolation polynomes.
///
/// We used seven consecutive locations (named x1 to x7) and compute
/// the following equations using
///
///    x1 = 3, x2 = 2, x3 = 1, x4 = 0, x5 = 1, x6 = 2, x7 = 3:
///
/// P1 = (x (x (x (x ((x - 3) x - 5) + 15) + 4) - 12)) / 720
/// P2 = (x (x (x (x ((2 - x) x + 10) - 20) - 9) + 18)) / 120
/// P3 = (x (x (x (x ((x - 1) x - 13) + 13) + 36) - 36)) / 48
//  P4 = (x² (x² (14 - x²) - 49) + 36) / 36
/// P5 = (x (x (x (x (x (x + 1) - 13) - 13) + 36) + 36)) / 48
/// P6 = (x (x (x (x ((- x - 2) x + 10) + 20) - 9) - 18)) / 120
/// P7 = (x (x (x (x (x (x + 3) - 5) - 15) + 4) + 12)) / 720
///
/// These equations are used in a "sliding window" for wich we only
/// compute interpolation located between x4 and x5.

#define P1(x) ((x * (x * (x * (x * ((x - 3.0f) * x - 5.0f) + 15.0f) + 4.0f) - 12.0f)) / 720.0f)
#define P2(x) ((x * (x * (x * (x * ((2.0f - x) * x + 10.0f) - 20.0f) - 9.0f) + 18.0f)) / 120.0f)
#define P3(x) ((x * (x * (x * (x * ((x - 1.0f) * x - 13.0f) + 13.0f) + 36.0f) - 36.0f)) / 48.0f)
#define P4(x) (((x * x) * ((x * x) * (14.0f - (x * x)) - 49.0f) + 36.0f) / 36.0f)
#define P5(x) ((x * (x * (x * (x * (x * (x + 1.0f) - 13.0f) - 13.0f) + 36.0f) + 36.0f)) / 48.0f)
#define P6(x) ((x * (x * (x * (x * ((- x - 2.0f) * x + 10.0f) + 20.0f) - 9.0f) - 18.0f)) / 120.0f)
#define P7(x) ((x * (x * (x * (x * (x * (x + 3.0f) - 5.0f) - 15.0f) + 4.0f) + 12.0f)) / 720.0f)

int Voice::getSamples(buffp buff, int length)
{
  int count = 0;

  //assert((note - sample->getPitch()) >= 0);

  assert(sampleBuff != NULL);
  assert(buff != NULL);
  assert(length > 0);

  while (length--) {

    float samplePos = ((float) outputPos) * (factor * synth.vibrato(outputPos));

    uint32_t integralPart   = samplePos;
    float    fractionalPart = samplePos - integralPart;

    int16_t buffIndex = (integralPart % BUFFER_SAMPLE_COUNT) - 3;

    if (integralPart >= (sampleBuffPos + sampleBuffSize)) {
      sampleBuffPos += sampleBuffSize;
      memcpy(sampleBuff, &sampleBuff[sampleBuffSize], 7 << LOG_SAMPLE_SIZE);
      if ((sampleBuffSize = retrieveFifoSamples(sampleBuff, 7)) == 0) break;
      assert((!synth.isLooping()) || (sampleBuffSize == BUFFER_SAMPLE_COUNT));
    }

    assert(buffIndex >= -3);

    float * y = &sampleBuff[buffIndex - 4 + 7];

    *buff++ = (y[0] * P1(fractionalPart)) +
              (y[1] * P2(fractionalPart)) +
              (y[2] * P3(fractionalPart)) +
              (y[3] * P4(fractionalPart)) +
              (y[4] * P5(fractionalPart)) +
              (y[5] * P6(fractionalPart)) +
              (y[6] * P7(fractionalPart));

    outputPos++;
    count++;
  }

  return count;
}

#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7

#endif
#endif


#if 0

//---- getSamples()
//
// This is called when the required samples must be reconstructed from
// another note data that is of a different pitch than the required one.
int Voice::getSamples(buffp buff, int length)
{
  int   count = 0;

  //assert((note - sample->getPitch()) >= 0);

  float factor = scaleFactors[(note - synth.getRootKey()) + 127] * synth.getCorrection();

  assert(sampleBuff != NULL);
  assert(buff != NULL);
  assert(length > 0);

  if (sampleBuffPos == -1) {
    if ((sampleBuffSize = getNormalSamples(sampleBuff)) == 0) return 0;
    sampleBuffPos = 0;
  }

  float pos = outputPos * factor;

  #if USE_NEON_INTRINSICS
    float aa[4], bb[4], cc[4];

    float  * a = aa;
    float  * b = bb;
    float  * c = cc;
    float old = 0;

    while (length--) {

      float fipos;

      float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part
      int   ipos = fipos - sampleBuffPos; // ipos = index in sampleBuff

      if (ipos >= 0) {
        while (ipos >= sampleBuffSize) {
          ipos -= sampleBuffSize;
          sampleBuffPos += sampleBuffSize;
          if ((sampleBuffSize = getNormalSamples(sampleBuff)) == 0) goto endLoop;
        }

        old = *a++ = sampleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        *a++ = old;

        assert(ipos == -1);
      }

      if (++ipos >= sampleBuffSize) {
        ipos -= sampleBuffSize;
        sampleBuffPos += sampleBuffSize;
        if ((sampleBuffSize = getNormalSamples(sampleBuff)) == 0) break;
      }

      // We do a linear approximation... not the best, but good enough...
      *b++ = sampleBuff[ipos];
      *c++ = diff;

      if ((++count & 3) == 0) {
        float32x4_t v1 = vld1q_f32(aa);
        float32x4_t v2 = vld1q_f32(bb);
        float32x4_t v3 = vld1q_f32(cc);

        v2 = vsubq_f32(v2, v1);
        v1 = vmlaq_f32(v1, v2, v3);

        vst1q_f32(buff, v1);

        buff += 4;
        a = aa;
        b = bb;
        c = cc;
      }

      pos += factor;
    }
  #else
    float a, old;

    old = 0.0;

    while (length--) {
      float fiposlength;
      float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part
      int   ipos = fipos - sampleBuffPos; // ipos = index in sampleBuff

      if (ipos >= 0) {
        while (ipos >= sampleBuffSize) {
          ipos -= sampleBuffSize;
          sampleBuffPos += sampleBuffSize;
          if ((sampleBuffSize = getNormalSamples(sampleBuff)) == 0) goto endLoop;
        }

        old = a = sampleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        a = old;
        assert(ipos == -1);
      }

      if (++ipos >= sampleBuffSize) {
        ipos -= sampleBuffSize;
        sampleBuffPos += sampleBuffSize;
        if ((sampleBuffSize = getNormalSamples(sampleBuff)) == 0) break;
      }

      // We do a linear approximation... not the best, but good enough...
      *buff++ = a + ((sampleBuff[ipos] - a) * diff);

      pos += factor;
      count++;
    }
  #endif

 endLoop:

  outputPos += count;
  return count;
}
#endif

#if 0
int Voice::getSamples(buffp buff, int length)
{
  int count = 0;

  assert(sampleBuff != NULL);
  assert(buff != NULL);
  assert(length > 0);

  // outputPos is the postion where we are in the output as a number
  // of samples since the start of the note. samplePos is where we need to
  // get someting from the sample, taking into account pitch changes, resampling
  // and modulation of all kind. buffIndex is the specific index in the
  // retrieved buffer.

  float lastSamplePos = ((float) outputPos) * factor;

  while (length--) {

    float samplePos = lastSamplePos + synth.vibrato(outputPos);

    lastSamplePos += factor;

    // The following is working as samplePos is a positive number...

    uint32_t integralPart = samplePos;
    float  fractionalPart = samplePos - integralPart;

    int16_t buffIndex = (integralPart % BUFFER_SAMPLE_COUNT) - 2;

    if (integralPart >= (sampleBuffPos + sampleBuffSize)) {
      sampleBuffPos += sampleBuffSize;

      // Retrieve the last 4 samples from the end of the buffer and put them at
      // the beginning to ensure proper interpolation at the beginning of next
      // buffer.
      //
      // For the first samples retrieval at the beginning of a note to be played,
      // the last 4 samples have been initialized to zero (0.0f) by the setup()
      // method.
      #if USE_NEON_INTRINSICS
        float32x4_t f = vld1q_f32(&sampleBuff[sampleBuffSize]);
        vst1q_f32(&sampleBuff[0], f);
      #else
        memcpy(sampleBuff, &sampleBuff[sampleBuffSize], 4 << LOG_SAMPLE_SIZE);
      #endif

      if ((sampleBuffSize = retrieveFifoSamples(sampleBuff, 4)) == 0) break;
      assert((!synth.isLooping()) || (sampleBuffSize == BUFFER_SAMPLE_COUNT));
    }

    assert(buffIndex >= -2);

    float * y = &sampleBuff[buffIndex - 1 + 4];

    *buff++ = y[1] + (y[2] - y[1]) * fractionalPart;

    outputPos++;
    count++;
  }

  return count;
}
#endif
