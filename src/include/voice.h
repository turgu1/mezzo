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

#ifndef VOICE_H
#define VOICE_H

#include "mezzo.h"

typedef enum { DORMANT, ALIVE } voiceState;

typedef class Voice * voicep;

// The 4 additional float in the buffer will allow for the continuity of
// interpolation between buffer retrieval action from the fifo. The last 4 samples
// of the last retrieved record will be put back as the 4 first samples in the buffer
// when a new record is read. See the feedBuffer() method for more information.

typedef std::array<sample_t, BUFFER_SAMPLE_COUNT + 4> scaleRecord;

/// Class Voice represents a sample that is currently playing through
/// the PCM device. (Class Sample represents the static portion of a
/// sample.)  Mezzo, at launch time, pre-allocates 512 voices that will
/// be associated to samples dynamically when a new sound will be
/// required to be played. The number of pre-allocated voices can be
/// easily changed in the Poly class definition (poly.h).

class Voice {

 private:
  voicep  next;              ///< Next voice available in the list

  uint32_t seq;
  static uint32_t nextSeq;
  static bool     showPlayingState;

  volatile voiceState state; ///< This is the state of this voice, as described in the comments above
  volatile bool  active;     ///< This voice is active and is being played
  volatile int   stateLock;  ///< Locked by threads when reading/updating data

  samplep     sample;        ///< Pointer on the sample
  int8_t      note;          ///< Targeted note, can be different than the one from sample
  bool        noteIsOn;      ///< The note is played
  bool        keyIsOn;       ///< The *keyboard* midi key is on
  float       gain;          ///< Gain to apply to this sample (depends on how the key was struck by the player)
  uint32_t    outputPos;     ///< Position in the scaled (or not) processed stream of samples

  #if loadInMemory
    buffp    sampleBuff;
    uint32_t sampleBuffSize;
  #else
    Fifo        fifo;        ///< Fifo for samples retrieved through threading
    scaleRecord sampleBuff;
    uint32_t    fifoLoadPos;
    uint16_t    sampleBuffSize;
  #endif

  uint32_t      sampleBuffPos;
  double        factor;

  sampleRecord  buffer;
  int           bufferSize;
  volatile bool bufferReady;

  Synthesizer   synth;

 public:
   Voice();
  ~Voice();

  static bool togglePlayingState()   { return showPlayingState = !showPlayingState; }
  static bool isPlayingStateActive() { return showPlayingState; }

  void showStatus(int spaces);

  /// Associate a sample with this voice. This will then activate this
  /// voice to be played.
  void setup(samplep       _sample,
             uint8_t       _note,
             float         _gain,
             Synthesizer   _synth,
             Preset      & _preset,
             uint16_t      _presetZoneIdx);

  #if !loadInMemory
    /// This method returns the next bundle of samples required by the
    /// mixer. The will be found in the associated fifo (first in, first out)
    /// buffer pre-loaded by the second thread.
    int retrieveFifoSamples(scaleRecord & buff, int pos);
  #endif

  /// This method returns the next scaled bundle of samples required by
  /// the mixer. The data is resampled in accordance with the shifted pitch (if requires)
  /// the Sample rate (in regard of the audio-out targetted sampling rate) and modulations.
  sampleRecord & getBuffer(int16_t * count);

  void releaseBuffer() { bufferReady = false; }

  /// Lock a multithreading resource that needs to be modified
  inline void BEGIN() { while (__sync_lock_test_and_set(&stateLock, 1)); }

  /// Unlock a multithreading resource that has been modified
  inline void END()   { __sync_lock_release(&stateLock); }

  inline bool isActive()   { return  active; }
  inline bool isInactive() { return !active; }

  inline bool isDormant()  { return state == DORMANT; }
  inline bool isAlive()    { return state == ALIVE;   }

  inline void setState(voiceState value) { state = value; };

  inline void activate()   { if (isInactive()) setState(ALIVE);   active = true;  }
  inline void inactivate() { if (isActive())   setState(DORMANT); active = false;
    #if !loadInMemory
      clearFifo();
    #endif
  }

  inline void    setNext(voicep n) { next = n;    }
  inline voicep  getNext()         { return next; }
  inline int8_t  getNote()         { return note; }
  inline float   getGain()         { return gain; }
  inline int16_t  getPan()         { return synth.getPan(); }
  inline uint32_t getSeq()         { return seq;  }

  static double getScaleFactor(int16_t diff);

  // At setup time, the bypass parameter allows to feed the buffer for
  // the first packet before being taken in charge by the feeding thread.
  // This is to ensure that the first packet is ready for consumption in
  // time for the poly::mixer method.
  void feedBuffer(bool bypass = false);

  #if !loadInMemory
    /// This method is used by the SampleFeeder thread to read new data
    /// from the sample and put it in the next avail slot in the
    /// fifo buffer, if there is some room available.
    void feedFifo();

    /// This is used to preload the fifo before activating the voice.
    void prepareFifo();

    inline void clearFifo() { fifo.clear();         }
  #endif

  inline void keyOff()    { keyIsOn = false;      }
  inline bool isKeyOn()   { return keyIsOn;       }
  inline bool isNoteOn()  { return noteIsOn;      }
  inline bool noteOff()   { keyIsOn = noteIsOn = false;
                            return synth.keyHasBeenReleased(); }

  inline bool transformAndMix(frameRecord & dst, sampleRecord & src, uint16_t length) {
    return synth.transformAndMix(dst, src, length);
  }
};

#endif
