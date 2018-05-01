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

void Voice::setup(samplep       sample,
                  char          note,
                  float         gain,
                  Synthesizer & synth,
                  Preset      & preset,
                  uint16_t      presetZoneIdx)
{
  // TODO: Why gain is squared??
  // Connect the sample with the voice
  this->sample   = sample;
  this->note     = note;
  this->synth    = synth;
  this->gain     = gain * synth.getAttenuation();

  samplePos      =  0;
  sampleRealPos  =  0;
  scaleBuffPos   = -1;

  fadingOut      = false;
  fadeOutPos     = 0;
  noteIsOn       = true;
  active         = false;
  fifoLoadPos    = 0;

  old_y1         =
  old_y2         =
  old_y3         =
  old_y4         = 0.0f;
  last_1st       = -1;

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

// #define P1 ((x_3 - 9 * x_2 + 26 * x - 24) / -6)
// #define P2 ((x_3 - 8 * x_2 + 18 * x - 12) /  2)
// #define P3 ((x_3 - 7 * x_2 + 14 * x -  8) / -2)
// #define P4 ((x_3 - 6 * x_2 + 11 * x -  6) /  6)
//
// uint16_t lagrange4th(buffp buff, float factor, uint16_t len)
// {
//   float x_3, x_2, x;
//
//   *buff++ = y1 * P1 + y2 * P2 + y3 * P3 + y4 * P4;
//
// }

#if 1

#define P1 ((x_3 - (x_2 + x_2 + x_2) + (x + x)) / -6.0f)
#define P2 ((x_3 - (x_2 + x_2) - x + 2.0f) /  2.0f)
#define P3 ((x_3 - x_2 - (x + x)) / -2.0f)
#define P4 ((x_3 - x) / 6.0f)

using namespace std;
#include <iomanip>

int Voice::getScaledSamples(buffp buff, int sampleCount)
{
  int count = 0;

  float y1, y2, y3, y4;
  float x_3, x_2, x;

  //assert((note - sample->getPitch()) >= 0);

  float factor = scaleFactors[(note - synth.getRootKey()) + 127] * synth.getCorrection();

  if (sample->getSampleRate() != config.samplingRate) {
    factor *= (config.samplingRate / sample->getSampleRate());
  }

  assert(scaleBuff != NULL);
  assert(buff != NULL);
  assert(sampleCount > 0);

  if (scaleBuffPos == -1) {
    if (getNormalSamples(scaleBuff) == 0) return 0;
    scaleBuffPos = 0;
  }

  float pos = sampleRealPos * factor;

  float old;

  old = 0.0;

  y1 = old_y1;
  y2 = old_y2;
  y3 = old_y3;
  y4 = old_y4;

  int last_first = last_1st;

  while (sampleCount--) {

    float fipos;
    float diff = modff(pos, &fipos); //fipos = integral part, diff = fractional part
    int   ipos = fipos - scaleBuffPos; // ipos = index in scaleBuff

    if (last_first != ipos) {

      y1 = y2;

      if (ipos >= 0) {
        while (ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
          if (getNormalSamples(scaleBuff) == 0) goto endLoop;
          ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
          scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
        }

        y2 = old = scaleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        y2 = old;
        //assert(ipos == -1);
      }

      if (++ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
        if (getNormalSamples(scaleBuff) == 0) break;
        ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
        scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
      }
      y3 = scaleBuff[ipos];

      if (++ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
        if (getNormalSamples(scaleBuff) == 0) break;
        ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
        scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
      }
      y4 = scaleBuff[ipos];
    }
    else {
      // cout << '*' << endl;
    }

    last_first = ipos - 2;

    // cout << "idx:" << setw(5) << (sampleRealPos + count)
    //     << " pos:"  << setw(10) << pos
    //     << " diff:" << setw(10) << diff
    //     << " ipos:" << setw(6) << ipos
    //     << " last_first:" << setw(6) << last_first
    //     << " y1:" << setw(10) << y1
    //     << " y2:" << setw(10) << y2
    //     << " y3:" << setw(10) << y3
    //     << " y4:" << setw(10) << y4 << endl;

    x = diff;
    x_2 = x * x;
    x_3 = x_2 * x;

    *buff++ = y1 * P1 + y2 * P2 + y3 * P3 + y4 * P4;

    pos += factor;
    count++;
  }

 endLoop:
  old_y1 = y1;
  old_y2 = y2;
  old_y3 = y3;
  old_y4 = y4;
  last_1st = last_first;

  sampleRealPos += count;
  return count;
}

#else

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
    if (getNormalSamples(scaleBuff) == 0) return 0;
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
        while (ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
          if (getNormalSamples(scaleBuff) == 0) goto endLoop;
          ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
          scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
        }

        old = *a++ = scaleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        *a++ = old;

        assert(ipos == -1);
      }

      if (++ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
        if (getNormalSamples(scaleBuff) == 0) break;
        ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
        scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
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
        while (ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
          if (getNormalSamples(scaleBuff) == 0) goto endLoop;
          ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
          scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
        }

        old = a = scaleBuff[ipos];
      }
      else {
        // We already have loaded next samples. Expected ipos to be -1 and related to
        // the data already retrieved in the preceeding loop...
        a = old;
        assert(ipos == -1);
      }

      if (++ipos >= SAMPLE_BUFFER_SAMPLE_COUNT) {
        if (getNormalSamples(scaleBuff) == 0) break;
        ipos -= SAMPLE_BUFFER_SAMPLE_COUNT;
        scaleBuffPos += SAMPLE_BUFFER_SAMPLE_COUNT;
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
