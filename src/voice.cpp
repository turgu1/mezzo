#include "copyright.h"

#include <cmath>

#include "mezzo.h"
#include "voice.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

#define SCALE_FACTOR_COUNT  128     // In midi, there is a potential of 128 note values

/// This vector contains the scale factors required to modify the pitch of a note to obtain a targeted
/// note sound. Please look in method Voice::Voice() for initialization values.

PRIVATE float scaleFactors[SCALE_FACTOR_COUNT];
PRIVATE bool scaleFactorsInitialized = false;

//---- Voice() ----

Voice::Voice()
{
  setNewHandler(outOfMemory);

  active        = false;
  state         = DORMANT;
  stateLock     = 0;
  sample        = NULL;
  samplePos     = 0;
  framePos      = 0;
  fadingOut     = false;
  fadeOutPos    = 0;
  noteIsOn      = false;
  gain          = 1.0f;
  fifo          = new Fifo;
  next          = NULL;

  scaleBuff = new sample_t[BUFFER_SAMPLE_COUNT];

  scaleBuffPos = -1;

  if (!scaleFactorsInitialized) {
    scaleFactorsInitialized = true;

    // As per "Twelve-tone equal temperament" described on the internet:
    //   https://en.wikipedia.org/wiki/Equal_temperament

    for (int i = 0; i < SCALE_FACTOR_COUNT; i++) {
      scaleFactors[i] = pow(2, i / 12.0);
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

void Voice::setup(samplep sample, char note, float gain)
{
  // Connect the sample with the voice
  this->sample   = sample;
  this->note     = note;
  this->gain     = gain * gain; // Gain is squared
  samplePos      = 0;
  framePos       = 0;
  scaleBuffPos   = -1;

  fadingOut      = false;
  fadeOutPos     = 0;
  noteIsOn       = true;
  active         = false;

  fifo->clear();

  BEGIN();
    activate();     // Must be the last flag set. The threads are watching it...
  END();
}

//---- getNormalFrames() ----

int Voice::getNormalFrames(buffp buff, int frameCount)
{
  int readFrameCount;

  if (isInactive()) {
    logger.DEBUG("Voice inactive in getFrames()!");
    return 0;
  }

  if (samplePos >= sample->getFrameCount()) {
    // We already have read all cached data from sample structure. It's now
    // time to get remaining data from the fifo circle
    if (!fifo->isEmpty()) {
      memcpy(buff,
	     fifo->getHead(),
	     (readFrameCount = fifo->getFrameCount()) << LOG_FRAME_SIZE);
      fifo->pop();
    }
    else {
      // No more data available or the thread was not fast enough to get data on time
      readFrameCount = 0;
      logger.DEBUG("Underrun!!!");
      fifo->showState();
      logger.FATAL("Stop!");
    }
  }
  else {
    // Read data from the cached buffer in the sample structure.
    if ((samplePos + frameCount) > sample->getFrameCount()) {
      readFrameCount = sample->getFrameCount() - samplePos;
    }
    else {
      readFrameCount = frameCount;
    }

    memcpy(buff,
           &sample->getBuff()[samplePos << 1],
           readFrameCount << LOG_FRAME_SIZE);
  }

  samplePos += readFrameCount;
  return readFrameCount;
}

//---- getScaledFrames()
//
// This is called when the required samples must be reconstructed from
// another note data that is lower in pitch than the required one.
int Voice::getScaledFrames(buffp buff, int frameCount)
{
  int   count = 0;

  assert((note - sample->getNote()) >= 0);

  float factor = scaleFactors[note - sample->getNote()];

  assert(scaleBuff != NULL);
  assert(buff != NULL);
  assert(frameCount > 0);

  if (scaleBuffPos == -1) {
    if (getNormalFrames(scaleBuff, BUFFER_FRAME_COUNT) == 0) return 0;
    scaleBuffPos = 0;
  }

  float pos = framePos * factor;

  float aa[4], bb[4], cc[4];

  float  * a = aa;
  float  * b = bb;
  float  * c = cc;
  float old1 = 0, old2 = 0;

  while (frameCount--) {

    float fipos;

    float diff = modff(pos, &fipos);
    int   ipos = fipos - scaleBuffPos;
    int   idx;

    if (ipos >= 0) {
      while (ipos >= BUFFER_FRAME_COUNT) {
        if (getNormalFrames(scaleBuff, BUFFER_FRAME_COUNT) == 0) goto endLoop;
        ipos -= BUFFER_FRAME_COUNT;
        scaleBuffPos += BUFFER_FRAME_COUNT;
      }

      old1 = *a++ = scaleBuff[idx = (ipos << 1)];
      old2 = *a++ = scaleBuff[idx + 1];
    }
    else {
      // We already have loaded next frames. Expected ipos to be -1 and related to
      // the data already retrieved in the preceeding loop...
      *a++ = old1;
      *a++ = old2;

      assert(ipos == -1);
    }

    if (++ipos >= BUFFER_FRAME_COUNT) {
      if (getNormalFrames(scaleBuff, BUFFER_FRAME_COUNT) == 0) break;
      ipos -= BUFFER_FRAME_COUNT;
      scaleBuffPos += BUFFER_FRAME_COUNT;
    }

    // We do a linear approximation... not the best, but good enough...
    *b++ = scaleBuff[idx = (ipos << 1)];
    *b++ = scaleBuff[idx + 1];
    *c++ = diff;
    *c++ = diff;

    if ((++count & 1) == 0) {
      #if USE_NEON_INTRINSICS
        float32x4_t v1 = vld1q_f32(aa);
        float32x4_t v2 = vld1q_f32(bb);
        float32x4_t v3 = vld1q_f32(cc);

        v2 = vsubq_f32(v2, v1);
        v1 = vmlaq_f32(v1, v2, v3);

        vst1q_f32(buff, v1);

        buff += 4;
      #else
        for (int i = 0; i < 3; i++) {
          *buff++ = aa[i] + ((bb[i] - aa[i]) * cc[i]);
        }
      #endif
      a = aa;
      b = bb;
      c = cc;
    }

    pos += factor;
  }

 endLoop:

  framePos += count;

  return count;
}

void Voice::showState()
{
  using namespace std;

  PRIVATE const char *  stateStr[4] = { "DORMANT", "OPENING", "ALIVE", "CLOSING" };

  cout << "> act:"   << (active ? "true" : "false")             << " "
       << "state:"   << (stateStr[state])                       << " "
       << "pos:"     << (framePos)                              << " "
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
