#include "copyright.h"

#ifndef VOICE_H
#define VOICE_H

#include "fifo.h"
#include "sample.h"

typedef enum { DORMANT, ALIVE } voiceState;

typedef class Voice * voicep;

/// Class Voice represents a sample that is currently playing through
/// the PCM device. (Class Sample represents the static portion of a
/// sample.)  Mezzo, at launch time, pre-allocate 256 voices that will
/// be associated to samples dynamically when a new sound will be
/// required to be played. The number of pre-allocated voices can be
/// easily changed in the Poly class definition (poly.h).
///
/// A voice (an instance of the class Voice), during its life, goes
/// through a variety of states that enable the synchronization of
/// events that occurs in 4 concurrent threads:
///
///  o Midi callback processing (when a user interact with the
///    keyboard)
///
///  o PCM callback processing (when the PCM device requires new sound
///    data)
///
///  o In the Samples Feeder thread (when its time to feed new sample
///    data in the FIFO buffer)
///
/// As you may have inferred, the Voice class is at the center of the
/// activities allowing the generation of sounds to be sent to the PCM
/// device.
///
/// The main duty of an instance of this class is to supply sound data
/// from a single Sample object to the Poly::mixer method in bundles of
/// 256 samples. When a voice is first activated, it starts by
/// supplying samples that have been pre-loaded at the time the Samples
/// class has been instantiated (at program startup or when a new
/// Preset is loaded). This will give (hopefully)
/// sufficient time to background threads to
/// start to feed a FIFO (First In, First Out) buffer.  At the end of
/// the pre-loaded samples buffer, the instance will then start to read
/// data from the FIFO until the end of the sample or a request by the
/// user (through MIDI) to stop a note being played.
///
/// Class Poly will retrieve samples buffer from voices to build
/// frames. A frame contains two float values, one for each side of a
/// stereophonic sound. The PCM device is programmed to require 44100
/// frames per second. A bundle will supply ~5.8ms (256 / 44100) of
/// data to the PCM device. Once a request is received by the
/// application, the maximum latency to produce changes is then less
/// than or equal to 5.8ms (A note is requested by the user: at the
/// time the note is struck on the keyboard, there is already a
/// bundle being played, a second bundle is in preparation by Mezzo,
/// the note tricked will be taken into account no later than in the
/// next bundle to be prepared).
///
/// As is often the case, a Samples Library maybe lacking some of the
/// notes required to fulfill the entire keyboard keys. This class
/// allow for scaling of notes to reflect the pitch of the targeted
/// note.
///
/// \section VoiceDetails On the dynamics of the Voice Class
///
/// The following state diagram presents the various states through
/// which a voice is going through when a sound is required to be
/// played.
///
/// \startuml
///     [*] -down-> Inactive
///
///     state Inactive {
///       [*] -down-> DORMANT
///       CLOSING -up-> DORMANT
///       CLOSING : FadeOut completed
///       DORMANT : Voice is not in use
///     }
///     state Active {
///       OPENING -down-> ALIVE
///       OPENING : A Sample is attached to the voice
///       OPENING : Pre-loaded PCM data supplied to mixer
///       ALIVE : Fifo filled continuously
///       ALIVE : SamplesFeeder thread is in charge
///       ALIVE : noteOff... FadeOut activated
///     }
///
///     DORMANT -right-> OPENING : (Voice activated)
///     ALIVE   -left-> CLOSING : (FadeOut complete)
///     OPENING --> CLOSING : (FadeOut complete)
/// \enduml
///
/// A voice will be played only when its global state is ACTIVE. At
/// startup, all voices are DORMANT and INACTIVE.  When a sound is
/// requested, a voice is activated by associating it to a sample and
/// changing its state to OPENING and ACTIVE. The state is changed
/// to ALIVE. The FIFO associated with the voice is then being feed
/// by the SamplesFeeding thread. When a request is made to stop to
/// play a voice, the sound is fade out, diminishing its volume to
/// zero for the next second or the end of the sample data if it comes
/// before. When fade out is complete, the voice states is then put as
/// INACTIVE and CLOSING.
///
/// Fade out processing is done by the Poly::mixer() method, as it
/// implies an optimization on computations done with the ARM NEON
/// intrinsic instructions.

class Voice : public NewHandlerSupport<Voice> {

 private:
  voicep  next;              ///< Next voice available in the list

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
  
  Synthesizer synth;

  static void outOfMemory(); ///< New operation handler when out of memory occurs

 public:
   Voice();
  ~Voice();

  void showState();

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
  int getSamples(buffp buff, int length);

  inline void BEGIN() { while (__sync_lock_test_and_set(&stateLock, 1)); } ///< Lock a multithreading resource that needs to be modified
  inline void END()   { __sync_lock_release(&stateLock); } ///< Unlock a multithreading resource that has been modified

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

  static float getScaleFactor(int16_t diff);

  /// This method is used by the SampleFeeder thread to read new data
  /// from the sample and put it in the next avail slot in the
  /// fifo buffer, if there is some room available.
  void feedFifo();

  /// This is used to preload the fifo bewfor activiating the voice.
  void prepareFifo();

  inline void clearFifo()       { fifo->clear();        }
  inline void noteOff()         { keyIsOn = noteIsOn = false; synth.keyHasBeenReleased(); }
  inline void keyOff()          { keyIsOn = false;      }  
  inline bool isNoteOn()        { return noteIsOn;      }
  inline bool isKeyOn()         { return keyIsOn;       }

  inline bool transform(buffp tmpBuff, buffp voiceBuff, uint16_t length) {
    return synth.transform(tmpBuff, voiceBuff, length); }
};

#endif
