#include "copyright.h"

#ifndef VOICE_H
#define VOICE_H

#include "fifo.h"
#include "sample.h"

typedef enum { DORMANT, ALIVE } voiceState;

typedef class Voice * voicep;

/// Class Voice represents a sample that is currently playing through
/// the PCM device. (Class Sample represents the static portion of a
/// sample.)  Mezzo, at launch time, pre-allocates 512 voices that will
/// be associated to samples dynamically when a new sound will be
/// required to be played. The number of pre-allocated voices can be
/// easily changed in the Poly class definition (poly.h).

class Voice : public NewHandlerSupport<Voice> {

 private:
  voicep  next;              ///< Next voice available in the list
  
  uint32_t seq;
  static  uint32_t nextSeq;

  static bool showPlayingState;

  volatile voiceState state; ///< This is the state of this voice, as described in the comments above
  volatile bool  active;     ///< This voice is active and is being played
  volatile int   stateLock;  ///< Locked by threads when reading/updating data

  samplep     sample;        ///< Pointer on the sample
  int8_t      note;          ///< Targeted note, can be different than the one from sample
  bool        noteIsOn;      ///< The note is played
  bool        keyIsOn;       ///< The *keyboard* midi key is on
  float       gain;          ///< Gain to apply to this sample (depends on how the key was struck by the player)
  uint32_t    outputPos;     ///< Position in the scaled (or not) processed stream of samples
  Fifo      * fifo;          ///< Fifo for samples retrieved through threading
  buffp       scaleBuff;     ///< Used when scaling must be done (voice note != sample note)
  uint32_t    scaleBuffPos;
  uint16_t    scaleBuffSize;
  uint32_t    fifoLoadPos;
  float       factor;

  buffp       buffer;
  int         bufferSize;
  volatile bool bufferReady;
  
  Synthesizer synth;

  static void outOfMemory(); ///< New operation handler when out of memory occurs

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

  /// This method returns the next bundle of samples required by the
  /// mixer. The will be found in the associated fifo (first in, first out)
  /// buffer pre-loaded by the second thread.
  int retrieveFifoSamples(buffp buff);

  /// This method returns the next scaled bundle of samples required by
  /// the mixer. The data is resample in accordance with the shifted pitch (if requires)
  /// the Sample rate (in regard of the audio-out targetted sampling rate) and modulations. 
  int getBuffer(buffp *buff);
  void releaseBuffer(bool resetPos = false);

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
  inline void inactivate() { if (isActive())   setState(DORMANT); active = false; clearFifo(); }

  inline void    setNext(voicep n) { next = n; }
  inline voicep  getNext() { return next; }
  inline int8_t  getNote() { return note; }
  inline float   getGain() { return gain; }
  inline int16_t  getPan() { return synth.getPan(); }
  inline uint32_t getSeq() { return seq; }

  inline buffp   getBuffer() { return buffer; }
  inline int     getBufferSize() { return bufferSize; }

  static float getScaleFactor(int16_t diff);

  /// This method is used by the SampleFeeder thread to read new data
  /// from the sample and put it in the next avail slot in the
  /// fifo buffer, if there is some room available.
  void feedFifo();

  // At setup time, the bypass parameter allows to feed the buffer for
  // the first packet before being taken in charge by the feeding thread.
  // This is to ensure that the first packet in ready for consumption in
  // time for the poly::mixer method.
  void feedBuffer(bool bypass = false);

  /// This is used to preload the fifo bewfor activiating the voice.
  void prepareFifo();

  inline void clearFifo() { fifo->clear();        }
  inline void keyOff()    { keyIsOn = false;      }  
  inline bool isKeyOn()   { return keyIsOn;       }
  inline bool isNoteOn()  { return noteIsOn;      }
  inline bool noteOff()   { keyIsOn = noteIsOn = false; 
                            return synth.keyHasBeenReleased(); }

  inline bool transformAndAdd(buffp dst, buffp src, uint16_t length) {
    return synth.transformAndAdd(dst, src, length, gain * config.masterVolume); }
};

#endif
