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
          *synth
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
      *synth
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
  synth         = NULL;

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
                  Synthesizer & synth)
{
  // TODO: Why gain is squared??
  // Connect the sample with the voice
  this->sample   = sample;
  this->note     = note;
  this->synth    = &synth;
  this->gain     = gain * synth.getAttenuation();

  samplePos      =  0;
  sampleRealPos  =  0;
  scaleBuffPos   = -1;

  fadingOut      = false;
  fadeOutPos     = 0;
  noteIsOn       = true;
  active         = false;
  fifoLoadPos    = 0;

  prepareFifo();

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
