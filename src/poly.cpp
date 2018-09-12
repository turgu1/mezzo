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
#include <sched.h>
#include <pthread.h>

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <iomanip>

#include "mezzo.h"

pthread_cond_t  voiceCond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t voiceMutex = PTHREAD_MUTEX_INITIALIZER;

void stopThreads()
{
  keepRunning = false;
  pthread_cond_broadcast(&voiceCond);
}

//---- samplesFeeder() ----
//
// This function represent a thread responsible of reading
// samples from the SoundFont to push into the voices structure close to real-time.

void * samplesFeeder(void * args)
{
  (void) args;

  while (keepRunning) {

    if (poly->getVoiceCount() == 0) {
      pthread_cond_wait(&voiceCond, &voiceMutex);
      pthread_mutex_unlock(&voiceMutex);
    } 
    
    voicep voice = poly->getVoices();

    while ((voice != NULL) && keepRunning) {

      voice->feedFifo();

      sched_yield();
      voice = voice->getNext();
    }
  }

  pthread_exit(NULL);
}

//---- voicesFeeder() ----
//
// This function represent a thread responsible of readying a voice buffer packet
// on time for consumption by the poly::mixer method.

void * voicesFeeder1(void * args)
{
  (void) args;

  while (keepRunning) {

    if (poly->getVoiceCount() == 0) {
      pthread_cond_wait(&voiceCond, &voiceMutex);
      pthread_mutex_unlock(&voiceMutex);
    } 
    
    voicep voice = poly->getVoices();

    while ((voice != NULL) && (voice->getSeq() & 0x01)) voice = voice->getNext();

    while ((voice != NULL) && keepRunning) {

      voice->feedBuffer();

      sched_yield();
      do {
        voice = voice->getNext();
      } while ((voice != NULL) && (voice->getSeq() & 0x01));
    }
  }

  pthread_exit(NULL);
}

void * voicesFeeder2(void * args)
{
  (void) args;

  while (keepRunning) {

    if (poly->getVoiceCount() == 0) {
      pthread_cond_wait(&voiceCond, &voiceMutex);
      pthread_mutex_unlock(&voiceMutex);
    } 

    voicep voice = poly->getVoices();

    while ((voice != NULL) && ((voice->getSeq() & 0x01) == 0)) voice = voice->getNext();

    while ((voice != NULL) && keepRunning) {

      voice->feedBuffer();

      sched_yield();
      do {
        voice = voice->getNext();
      } while ((voice != NULL) && ((voice->getSeq() & 0x01) == 0));
    }
  }

  pthread_exit(NULL);
}

#define MONITOR_WAIT_COUNT 5

void monitorPorts()
{
   midi->checkPort();
  //sound->checkPort(); // Not working
}

// ---- portMonitor() ----
//
// If not in interactive mode, this function monitor ports for reconnection every
// 5 seconds. It must be run within the main application thread. For some reason,
// running it in a pthread doesn't work.
//
// The function signature is the same as for a pthread, but is not used as such.

void * portMonitor(void * args)
{
  int count = MONITOR_WAIT_COUNT;

  while (keepRunning) {
    sleep(1);
    if (keepRunning && (count-- <= 0)) {
      count = MONITOR_WAIT_COUNT;
      monitorPorts();
    }
  }

  return NULL;
  // pthread_exit(NULL);
}

void Poly::showState()
{
  voicep voice = voices;
  int i = 0;

  using namespace std;

  cout << "[Voices Dump Start]" << endl;

  while (voice) {
    if (!voice->isDormant()) voice->showStatus(2);

    voice = voice->getNext();
    i++;
  }
  cout << "[Voices Dump End]" << endl << endl;
}

//---- Poly() ----
//
// TODO: Sequencing of required initializations after a new library is loaded, if
//       we ever wants to allow for library change without restarting Mezzo

Poly::Poly()
{
  setNewHandler(outOfMemory);

  voicep prev  = NULL;
  voicep voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++) {

    voice = new Voice();

    voice->setNext(prev);

    prev = voice;
  }

  voices = voice;

  voiceCount = maxVoiceCount = 0;
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

  int maxCount = maxVoiceCount;
  logger.INFO("Max Nbr of Voices used: %d.\n", maxCount);
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
  // bool unblockThreads = (voiceCount == 0);

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
  int theCount = voiceCount;
  if (theCount > maxVoiceCount) maxVoiceCount = theCount;

  voice->setup(sample, note, gain, synth, preset, presetZoneIdx);

  // if (unblockThreads) {
  //   pthread_cond_broadcast(&voiceCond);
  // }
}

void Poly::UnblockVoiceThreads()
{
  pthread_cond_broadcast(&voiceCond);
}

//---- noteOff() ----

void Poly::noteOff(char note, bool pedalOn)
{
  voicep voice = voices;
  while (voice) {
    if (voice->isActive() && (voice->getNote() == note)) {
      if (pedalOn) {
        voice->keyOff();   // Just to keep the key state
      }
      else {
        if (voice->noteOff()) {  // This will turn off both key and note
          // Come here usually when the envelope is inactive and there is no
          // other way to know the time to stop playing this voice. This won't
          // be good for the player as he/she will ear clicks from the speakers
          voice->BEGIN();
            voice->inactivate();
          voice->END();
          voiceCount--;
        }
      }
    }
    voice = voice->getNext();
  }
}

void Poly::voicesSustainOff()
{
  voicep voice = voices;
  while (voice) {
    // When we receive the signal that the sustain pedal is off, we
    // don't want to stop notes that are still strucked by the player
    if (voice->isActive() && !voice->isKeyOn()) voice->noteOff();
    voice = voice->getNext();
  }
}

//---- mixer() ----

# define MIX(a, b) (a + b)

int Poly::mixer(frameRecord & buff)
{
  int maxFrameCount = 0; // Max number of frames that have been mixed
  int mixedCount = 0;    // Running counter on how many voices have beed
                         //   mixed so far in the loop
  static frame_t zero = { 0.0f, 0.0f };

  std::fill(std::begin(buff), std::end(buff), zero);

  Duration *duration = new Duration(); // TODO: Keep a duration object forever

  voicep voice = firstVoice();

  while (voice != NULL) {

    int16_t count;
    sampleRecord & voiceBuff = voice->getBuffer(&count);

    if (count < 0) {
      std::cout << '.' << std::flush;
    }
    else if (count > 0) {

      bool endOfSound = voice->transformAndMix(buff, voiceBuff, count);

      voice->releaseBuffer();

      // if endOfSound, we are at the end of the envelope sequence
      // and will now get rid of the voice.
      if (endOfSound) {
        voice->BEGIN();
          voice->inactivate();
        voice->END();
        voiceCount--;
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
