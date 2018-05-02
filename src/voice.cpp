#include "copyright.h"

#include <cmath>

#include "mezzo.h"
#include "voice.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

// In midi, there is a potential of 128 note values
// The vector will get scale factors for offsets between two notes
// ranging  from -127 to +127
#define SCALE_FACTOR_COUNT  ((127 * 2) + 1)

/// This vector contains the scale factors required to modify the pitch of a note to obtain a targeted
/// note sound. Please look in method Voice::Voice() for initialization values.

PRIVATE bool scaleFactorsInitialized = false;
PRIVATE float scaleFactors[SCALE_FACTOR_COUNT];

float Voice::getScaleFactor(int16_t diff)
{
  return scaleFactors[diff + 127];
}

void Voice::feedFifo()
{
  if (isActive() && isAlive()) {
    if (!fifo->isFull()) {
      if (__sync_lock_test_and_set(&stateLock, 1) == 0) {
        uint16_t count = sample->getData(
          fifo->getTail(),
          fifoLoadPos,
          SAMPLE_BUFFER_SAMPLE_COUNT,
          synth
        );
        if (count) {
          fifoLoadPos += count;
          fifo->setSampleCount(count);
          fifo->push();
        }
        END();
      }
    }
  }
}

void Voice::prepareFifo()
{
  fifo->clear();
  // while (!fifo->isFull())
  for (int i = 0; i < 5; i++)
  {
    uint16_t count = sample->getData(
      fifo->getTail(),
      fifoLoadPos,
      SAMPLE_BUFFER_SAMPLE_COUNT,
      synth
    );
    if (count) {
      fifoLoadPos += count;
      fifo->setSampleCount(count);
      fifo->push();
    }
    else {
      return;
    }
  }
}

//---- Voice() ----

Voice::Voice()
{
  setNewHandler(outOfMemory);

  active        = false;
  state         = DORMANT;
  stateLock     = 0;
  sample        = NULL;
  samplePos     = 0;
  sampleRealPos = 0;
  fadingOut     = false;
  fadeOutPos    = 0;
  noteIsOn      = false;
  gain          = 1.0f;
  fifo          = new Fifo;
  next          = NULL;

  scaleBuff = new sample_t[SAMPLE_BUFFER_SAMPLE_COUNT];

  scaleBuffPos = -1;

  if (!scaleFactorsInitialized) {
    scaleFactorsInitialized = true;

    // As per "Twelve-tone equal temperament" described on the internet:
    //   https://en.wikipedia.org/wiki/Equal_temperament

    for (int i = 0; i < SCALE_FACTOR_COUNT; i++) {
      scaleFactors[i] = pow(2, (i - 127) / 12.0);
    }
  }
}

//---- ~Voice() ----

Voice::~Voice()
{
  delete fifo;
  delete [] scaleBuff;
}

//----- outOfMemory() ----

void Voice::outOfMemory()
{
  logger.FATAL("Voice: Unable to allocate memory.");
}

//---- setup() ----

void Voice::setup(samplep      sample,
                  char         note,
                  float        gain,
                  Synthesizer  synth,
                  Preset     & preset,
                  uint16_t     presetZoneIdx)
{
  // Connect the sample with the voice
  this->sample   = sample;
  this->note     = note;
  this->synth    = synth;
  this->gain     = gain * synth.getAttenuation();

  samplePos      =  0;
  sampleRealPos  =  0;
  scaleBuffPos   = -1;
  scaleBuffSize  =  0;

  fadingOut      = false;
  fadeOutPos     = 0;
  noteIsOn       = true;
  active         = false;
  fifoLoadPos    = 0;

  prepareFifo();

  synth.addGens(preset.getGlobalGens(),            preset.getGlobalGenCount());
  synth.addGens(preset.getZoneGens(presetZoneIdx), preset.getZoneGenCount(presetZoneIdx));

  BEGIN();
    activate();     // Must be the last flag set. The threads are watching it...
  END();
}

//---- getNormalSamples() ----

int Voice::getNormalSamples(buffp buff)
{
  int readSampleCount;

  if (isInactive()) {
    logger.DEBUG("Voice inactive in getSamples()!");
    return 0;
  }

  if (fifo->isEmpty()) {
    // No more data available or the thread was not fast enough to get data on time
    readSampleCount = 0;
    // logger.DEBUG("Underrun!!!");
    // fifo->showState();
    // logger.FATAL("Stop!");
  }
  else {
    memcpy(buff,
     fifo->getHead(),
     (readSampleCount = fifo->getSampleCount()) << LOG_SAMPLE_SIZE);
    fifo->pop();
  }

  samplePos += readSampleCount;
  return readSampleCount;
}


#if 0

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

P1(x) = ((x * (x * (x * (x * ((x - 3.0f) * x - 5.0f) + 15.0f) + 4.0f) - 12.0f)) / 720.0f)
P2(x) = ((x * (x * (x * (x * ((2.0f - x) * x + 10.0f) - 20.0f) - 9.0f) + 18.0f)) / 120.0f)
P3(x) = ((x (x (x (x ((x - 1.0f) x - 13.0f) + 13.0f) + 36.0f) - 36.0f)) / 48.0f)
P4(x) = ((x * x * (x * x * (14.0f - x * x) - 49.0f) + 36.0f) / 36.0f)
P5(x) = ((x * (x * (x * (x * (x * (x + 1.0f) - 13.0f) - 13.0f) + 36.0f) + 36.0f)) / 48.0f)
P6(x) = ((x * (x * (x * (x * ((- x - 2.0f) * x + 10.0f) + 20.0f) - 9.0f) - 18.0f)) / 120.0f)
P7(x) = ((x * (x * (x * (x * (x * (x + 3.0f) - 5.0f) - 15.0f) + 4.0f) + 12.0f)) / 720.0f)

#undef P1
#undef P2
#undef P3
#undef P4
#undef P5
#undef P6
#undef P7

#endif

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
/// (the last equation on the right are Horner's forms)
///
/// These equations are used in a "sliding window" for wich we only
/// compute interpolation located between x2 and x3.

#define P1(x) ((x * ((3.0f - x) * x - 2.0f)) / 6.0f)
#define P2(x) ((x * ((x - 2.0f) * x - 1.0f) + 2.0f) / 2.0f)
#define P3(x) ((x * ((1.0f - x) * x + 2.0f)) / 2.0f)
#define P4(x) ((x * ((x * x) - 1.0f)) / 6.0f)

using namespace std;
#include <iomanip>

int Voice::getScaledSamples(buffp buff, int sampleCount)
{
  int count = 0;

  float y1, y2, y3, y4;

  //assert((note - sample->getPitch()) >= 0);

  float factor = scaleFactors[(note - synth.getRootKey()) + 127] * synth.getCorrection();

  if (sample->getSampleRate() != config.samplingRate) {
    factor *= ((float)sample->getSampleRate() / (float)config.samplingRate);
  }

  assert(scaleBuff != NULL);
  assert(buff != NULL);
  assert(sampleCount > 0);

  if (scaleBuffPos == -1) {
    if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) return 0;
    scaleBuffPos = 0;
    ipos         = 0;
    last_first   = -1;
    old_y1       =
    old_y2       =
    old_y3       =
    old_y4       = 0.0f;
  }

  float pos = sampleRealPos * factor;
  float old = 0.0f;

  y1 = old_y1;
  y2 = old_y2;
  y3 = old_y3;
  y4 = old_y4;

  while (sampleCount--) {

    float fipos;
    float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part

    if (last_first != fipos) {

      if ((fipos - last_first) == 1) {
        y1 = y2;
        y2 = y3;
        y3 = y4;

        if (++ipos >= scaleBuffSize) {
          ipos = 0;
          scaleBuffPos += scaleBuffSize;
          if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) break;
        }
        y4 = scaleBuff[ipos];
      }
      else {
        ipos = fipos - scaleBuffPos;

        if ((fipos - last_first) == 2) {
          y1 = y3;
        }
        else if ((fipos - last_first) == 3) {
          y1 = y4;
        }
        else {
          //Need to used a larger order than 4...
          //logger.FATAL("Internal error. Scale factor too large: %8.4f (%d)", factor, fipos - last_first);
        }
        if (ipos >= 0) {
          if (ipos >= scaleBuffSize) {
            ipos = 0;
            scaleBuffPos += scaleBuffSize;
            if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) goto endLoop;
          }

          y2 = old = scaleBuff[ipos];
        }
        else {
          // We already have loaded next samples. Expected ipos to be -1 and related to
          // the data already retrieved in the preceeding loop...
          y2 = old;
          if (ipos != -1) logger.DEBUG("Oups, ipos = %d", ipos);
        }

        if (++ipos >= scaleBuffSize) {
          ipos = 0;
          scaleBuffPos += scaleBuffSize;
          if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) break;
         }
        y3 = scaleBuff[ipos];

        if (++ipos >= scaleBuffSize) {
          ipos = 0;
          scaleBuffPos += scaleBuffSize;
          if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) break;
        }
        y4 = scaleBuff[ipos];
      }

      last_first = fipos;
    }

    // cout << "idx:" << setw(5) << (sampleRealPos + count)
    //     << " pos:"  << setw(10) << pos
    //     << " diff:" << setw(10) << diff
    //     << " ipos:" << setw(6) << ipos
    //     << " last_first:" << setw(6) << last_first
    //     << " y1:" << setw(10) << y1
    //     << " y2:" << setw(10) << y2
    //     << " y3:" << setw(10) << y3
    //     << " y4:" << setw(10) << y4 << endl;

    *buff++ = (y1 * P1(diff)) + (y2 * P2(diff)) + (y3 * P3(diff)) + (y4 * P4(diff));

    pos += factor;
    count++;
  }

endLoop:
  old_y1 = y1;
  old_y2 = y2;
  old_y3 = y3;
  old_y4 = y4;

  sampleRealPos += count;
  return count;
}

#undef P1
#undef P2
#undef P3
#undef P4

#endif

#if 0

//---- getScaledSamples()
//
// This is called when the required samples must be reconstructed from
// another note data that is of a different pitch than the required one.
int Voice::getScaledSamples(buffp buff, int sampleCount)
{
  int   count = 0;

  //assert((note - sample->getPitch()) >= 0);

  float factor = scaleFactors[(note - synth->getRootKey()) + 127];

  assert(scaleBuff != NULL);
  assert(buff != NULL);
  assert(sampleCount > 0);

  if (scaleBuffPos == -1) {
    if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) return 0;
    scaleBuffPos = 0;
  }

  float pos = sampleRealPos * factor;

  #if USE_NEON_INTRINSICS
    float aa[4], bb[4], cc[4];

    float  * a = aa;
    float  * b = bb;
    float  * c = cc;
    float old = 0;

    while (sampleCount--) {

      float fipos;

      float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part
      int   ipos = fipos - scaleBuffPos; // ipos = index in scaleBuff

      if (ipos >= 0) {
        while (ipos >= scaleBuffSize) {
          ipos -= scaleBuffSize;
          scaleBuffPos += scaleBuffSize;
          if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) goto endLoop;
        }

        old = *a++ = scaleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        *a++ = old;

        assert(ipos == -1);
      }

      if (++ipos >= scaleBuffSize) {
        ipos -= scaleBuffSize;
        scaleBuffPos += scaleBuffSize;
        if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) break;
      }

      // We do a linear approximation... not the best, but good enough...
      *b++ = scaleBuff[ipos];
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

    while (sampleCount--) {
      float fipos;
      float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part
      int   ipos = fipos - scaleBuffPos; // ipos = index in scaleBuff

      if (ipos >= 0) {
        while (ipos >= scaleBuffSize) {
          ipos -= scaleBuffSize;
          scaleBuffPos += scaleBuffSize;
          if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) goto endLoop;
        }

        old = a = scaleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        a = old;
        assert(ipos == -1);
      }

      if (++ipos >= scaleBuffSize) {
        ipos -= scaleBuffSize;
        scaleBuffPos += scaleBuffSize;
        if ((scaleBuffSize = getNormalSamples(scaleBuff)) == 0) break;
      }

      // We do a linear approximation... not the best, but good enough...
      *buff++ = a + ((scaleBuff[ipos] - a) * diff);

      pos += factor;
      count++;
    }
  #endif

 endLoop:

  sampleRealPos += count;
  return count;
}
#endif

void Voice::showState()
{
  using namespace std;

  PRIVATE const char *  stateStr[4] = { "DORMANT", "OPENING", "ALIVE", "CLOSING" };

  cout << "> act:"   << (active ? "true" : "false")             << " "
       << "state:"   << (stateStr[state])                       << " "
       << "pos:"     << (sampleRealPos)                              << " "
       << "fadeout:" << (fadingOut ? "true" : "false")          << " "
       << "dp:"      << (fadeOutPos)                            << " "
       << "sample:"  << (sample == NULL ? "none" : "see below") << endl;

  cout << "   note:" << (note)                                  << " "
       << "gain:"    << (gain)                                  << " "
       << "sbuff:"   << (scaleBuff)                             << " "
       << "sbpos:"   << (scaleBuffPos)                          << " "
       << "fifo:"    << (fifo)                                  << endl;

  if (sample != NULL) sample->showState();
}
