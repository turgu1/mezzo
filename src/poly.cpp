#include "copyright.h"

#include <cmath>
#include <sched.h>
#include <pthread.h>

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <iomanip>

#include "mezzo.h"
#include "duration.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

//---- samplesFeeder() ----
//
// This function represent a thread responsible of reading
// samples from files to push into the voices structure in real-time,
// in support of the streaming of data to ALSA through the voice mixer()
// function.

void * samplesFeeder(void * args)
{
  (void) args;

  while (keepRunning) {

    voicep voice = poly->getVoices();

    while ((voice != NULL) && keepRunning) {

      voice->feedFifo();

      sched_yield();
      voice = voice->getNext();
    }
  }

  pthread_exit(NULL);
}

//---- showState() ----

void Poly::showState()
{
  voicep voice = voices;
  int i = 0;

  using namespace std;

  cout << "[Voices Dump Start]" << endl;

  while (voice) {
    if (!voice->isDormant()) voice->showState();

    voice = voice->getNext();
    i++;
  }
  cout << "[Voices Dump End]" << endl << endl;
}

//---- Poly() ----
//
// TODO: Sequencing of required initializations after a new library is loaded...

Poly::Poly()
{
  setNewHandler(outOfMemory);

  voicep prev = NULL;
  voicep voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++) {

    voice = new Voice();

    voice->setNext(prev);

    prev = voice;
  }

  voices = voice;

  voiceCount = maxVoiceCount = 0;

  tmpBuff   = new sample_t[ FRAME_BUFFER_SAMPLE_COUNT];
  voiceBuff = new sample_t[SAMPLE_BUFFER_SAMPLE_COUNT];

  memset(tmpBuff,   0, FRAME_BUFFER_SIZE );
  memset(voiceBuff, 0, SAMPLE_BUFFER_SIZE);
}

//---- ~Poly() ----

Poly::~Poly()
{
  voicep voice = voices;

  while (voice) {
    voice->BEGIN();
      voice->inactivate();
    voice->END();

    voicep next = voice->getNext();

    delete voice;

    voice = next;
  }

  delete [] tmpBuff;
  delete [] voiceBuff;

  logger.INFO("Max Nbr of Voices used: %d.\n", maxVoiceCount);
}

//----- outOfMemory() ----

void Poly::outOfMemory()
{
  logger.FATAL("Poly: Unable to allocate memory.");
}

//---- inactivateAllVoices()

void Poly::inactivateAllVoices()
{
  //logger.INFO("-->inactivateAllVoice()");

  voicep voice = voices;

  while (voice) {
    voice->BEGIN();
      voice->inactivate();
    voice->END();

    voice = voice->getNext();
  }
  voiceCount = 0;
}

//---- firstVoice() ----
//
// Returns the first active voice present int the voices list.

voicep Poly::firstVoice()
{
  voicep voice = voices;

  while (voice) {
    if (voice->isActive()) {
      break;
    }
    voice = voice->getNext();
  }
  return voice;
}

//---- nextVoice() ----
//
//  Returns the next active voice present in the voices list, else NULL.

voicep Poly::nextVoice(voicep prev)
{
  if (prev == NULL) return NULL;

  voicep voice = prev->getNext();

  while (voice) {
    if (voice->isActive()) {
      break;
    }
    voice = voice->getNext();
  }
  return voice;
}

//----- nextAvailable() -----
//
// TODO: Better way of finding next available slot...

voicep Poly::nextAvailable()
{
  voicep voice = voices;

  while (voice) {
    if (voice->isInactive() && voice->isDormant()) {
      break;
    }

    voice = voice->getNext();
  }
  return voice;
}

//---- addVoice() ----
//
// A sample is added to voices. If there is no more voice structure available,
// retrieve the oldest one from the current voices list.

void Poly::addVoice(samplep       sample,
                    uint8_t       note,
                    float         gain,
                    Synthesizer & synth,
                    Preset      & preset,
                    uint16_t presetZoneIdx)
{
  voicep voice;

  // Do not transpose beyond 12 semi-tone...

  // if (sample->getSampleRate() != 44100) {
  //   logger.DEBUG("sample %s at rate %d", sample->getName().c_str(), sample->getSampleRate());
  // }

  //noteOff(note, false);

  if (sample == NULL) {
    logger.DEBUG("Hum... sample is NULL...");
    return;
  }

  if ((voice = nextAvailable()) == NULL) {
    // TODO: Get rid of the oldest voice and take the place
    logger.ERROR("Overrun!");
    return;
  }

  // Adjust Number of active voices
  voiceCount++;
  if (voiceCount > maxVoiceCount) maxVoiceCount = voiceCount;

  voice->setup(sample, note, gain, synth, preset, presetZoneIdx);
}

//---- noteOff() ----

void Poly::noteOff(char note, bool pedalOn)
{
  voicep voice = voices;
  while (voice) {
    if (voice->isActive() && (voice->getNote() == note)) {
      if (!pedalOn) voice->noteOff();
    }
    voice = voice->getNext();
  }
}

void Poly::voicesSustainOff()
{
  voicep voice = voices;
  while (voice) {
    if (voice->isActive() && voice->isNoteOn()) voice->noteOff();
    voice = voice->getNext();
  }
}

//---- mixer() ----

# define MIX(a, b) (a + b)

int Poly::mixer(buffp buff, int frameCount)
{
  int maxFrameCount = 0; // Max number of frames that have been mixed
  int i;                 // Running counter on frames
  int mixedCount = 0;    // Running counter on how many voices have beed
                         //   mixed so far in the loop

  memset(buff, 0, frameCount << LOG_FRAME_SIZE);

  Duration *duration = new Duration();

  voicep voice = firstVoice();

  while (voice != NULL) {

    int count = voice->getSamples(voiceBuff, frameCount);

    if (count > 0) {

      bool endOfSound = voice->transform(tmpBuff, voiceBuff, count);

      buffp buffOut = buff;
      buffp buffIn  = tmpBuff;

      float   voiceGain = voice->getGain() * config.masterVolume;

      //if (voiceGain > 1.0) logger.DEBUG("Gain: %7.3f", voiceGain);

      #if USE_NEON_INTRINSICS

        // This is an ARM NEON optimized mixing algorithm. We gain a
        // factor 4 of performance improvement using those vectorized
        // instructions. Very neat! This will allow us to add new features
        // in a near future... (reverb, equalizer, filters, etc...)

        // Ensure that there is a multiple of 4 floating point samples in the buffer
        while (count & 1) {
          tmpBuff[count << 1] = 0;
          tmpBuff[(count++ << 1) + 1] = 0.0;
        }
        i = count >> 1;

        while (i--) {
          float32x4_t vecOut = vld1q_f32(buffOut);
          float32x4_t vecIn = vld1q_f32(buffIn);

          vecIn = vmulq_n_f32(vecIn, voiceGain);
          vecOut = vaddq_f32(vecOut, vecIn);
          vst1q_f32(buffOut, vecOut);

          buffOut += 4;
          buffIn += 4;
        }

      #else

        i = count;  // This will be our running counter

        sample_t  a, b;

        while (i--) {
          a = *buffOut;
          b = *buffIn++ * voiceGain;
          *buffOut++ = MIX(a, b);

          a = *buffOut;
          b = *buffIn++ * voiceGain;
          *buffOut++ = MIX(a, b);
        }

      #endif

      if (endOfSound) {
        voice->BEGIN();
          voice->inactivate();
        voice->END();
      }

      maxFrameCount = MAX(maxFrameCount, count);
    }
    else {
      // There is no more frames available so we desactivate the current voice.
      voice->BEGIN();
        voice->inactivate();
      voice->END();
      voiceCount--;
    }

    mixedCount += 1;

    voice = nextVoice(voice);
  }

  maxVoicesMixed = MAX(maxVoicesMixed, mixedCount);

  long dur = duration->getElapse();

  //std::cout << "<" << dur << ">" << std::endl;

  mixerDuration = MAX(mixerDuration, dur);
  delete duration;

  return maxFrameCount;
}

void Poly::monitorCount()
{
  char ch;

  // Setup unbuffered nonechoed character input for stdin
  struct termios old_tio, new_tio;
  tcgetattr(STDIN_FILENO, &old_tio);
  new_tio = old_tio;
  new_tio.c_lflag &= (~ICANON & ~ECHO);
  new_tio.c_cc[VMIN]  = 0;
  new_tio.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

  using namespace std;

  cout << endl << endl;
  cout << "VOICE COUNT MONITORING" << endl;
  cout << "Press any key to stop:" << endl << endl;

  while (read(STDIN_FILENO, &ch, 1) == 0) {
    cout << "[ " << voiceCount << " ]    \r" << flush;
    sleep(1);
  }
  cout << endl << endl;

  // Restore buffered echoed character input for stdin
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}
