#include "copyright.h"

#ifndef VOICE_H
#define VOICE_H

#include <sndfile.h>

#include "fifo.h"

typedef enum { DORMANT, OPENING, ALIVE, CLOSING } voiceState;

typedef class Voice * voicep;

/// Class Voice represents a sample that is currently playing through
/// the PCM device. (Class Sample represents the static portion of a
/// sample.)  PIano, at launch time, pre-allocate 128 voices that will
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
///  o In the Sample File Opener thread (when its time to open a
///    sample file to read more data)
///
///  o In the Samples Feeder thread (when its time to feed new sample
///    data in the FIFO buffer)
///
/// As you may have inferred, the Voice class is at the center of the
/// activities allowing the generation of sounds to be sent to the PCM
/// device.
///
/// The main duty of an instance of this class is to supply sound data
/// from a single sample file to the Poly::mixer method in bundles of
/// 256 frames. When a voice is first activated, it starts by
/// supplying frames that have been pre-loaded at the time the Samples
/// Library has been instantiated (at program startup or when a new
/// library is loaded in memory). This will give (hopefully)
/// sufficient time to background threads to open the sample file and
/// start to feed a FIFO (First In, First Out) buffer.  At the end of
/// the pre-loaded frame buffer, the instance will then start to read
/// data from the FIFO until the end of the sample or a request by the
/// user (through MIDI) to stop a note being played.
///
/// A frame contains two float values, one for each side of a
/// stereophonic sound. The PCM device is programmed to require 44100
/// frames per second. A bundle will supply ~5.8ms (256 / 44100) of
/// data to the PCM device. Once a request is received by the
/// application, the maximum latency to produce changes is then less
/// than or equal to 11.6ms (A note is requested by the user: at the
/// time the note is struck on the keyboard, there is already a
/// bundle being played, a second bundle is in preparation by PIano,
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
///       CLOSING -up-> DORMANT : (File is closed)
///       CLOSING : FadeOut completed
///       CLOSING : SampleFileOpener thread is in charge
///       DORMANT : Voice is not in use
///     }
///     state Active {
///       OPENING -down-> ALIVE : (File is open)
///       OPENING : A Sample is attached to the voice
///       OPENING : Pre-loaded PCM data supplied to mixer
///       OPENING : SampleFileOpener thread is in charge
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
/// changing its state to OPENING and ACTIVE. Once the file
/// associated with the sample has been opened, the state is changed
/// to ALIVE. The FIFO associated with the voice is then being feed
/// by the SamplesFeeding thread. When a request is made to stop to
/// play a voice, the sound is fade out, diminishing its volume to
/// zero for the next second or the end of the sample data if it comes
/// before. When fade out is complete, the voice states is then put as
/// INACTIVE and CLOSING. If the voice's fade out is completed before
/// the file has been opened (state ALIVE), it will then be put in the
/// CLOSING state anyway.
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

  SNDFILE * soundFile;       ///< Sound file while it is open to read samples
  bool    soundFileOpen;     ///< Sound file is open
  samplep sample;            ///< Pointer on the sample
  char    note;              ///< Targeted note, can be different than the one from sample
  bool    fadingOut;         ///< The note is being fade-out after key/pedal release
  bool    noteIsOn;          ///< The note is played
  float   gain;              ///< Gain to apply to this sample (depends on how the key was struck by the player)
  int     fadeOutPos;        ///< Index in the fade-out gain vector
  int     samplePos;         ///< Position in the sample stream of frames
  int     framePos;          ///< Position in the scaled (or not) stream of frames
  Fifo  * fifo;              ///< Fifo for samples retrieved through threading
  buffp   scaleBuff;         ///< Used when scaling must be done (voice note != sample note)
  int     scaleBuffPos;

  static void outOfMemory(); ///< New operation handler when out of memory occurs

 public:
   Voice();
  ~Voice();

  void showState();

  /// Associate a sample with this voice. This will then activate this
  /// voice to be played.
  void setup(samplep sample, char note, float gain);

  /// This method returns the next bundle of frames required by the
  /// mixer. Normal means that the note to be played is the same as
  /// the main frequency of the sample note.
  int getNormalFrames(buffp buff, int frameCount);

  /// This method returns the next scaled bundle of frames required by
  /// the mixer. The data is scaled as the note from the sample is not
  /// the same as the note data to be supplied by the voice. The
  /// scaling algorithm has been optimized with ARM NEON Intrinsic
  /// instructions.
  int getScaledFrames(buffp buff, int frameCount);

  /// This inline method selects the right method to use depending on
  /// the note being played by the voice to supply scaled or
  /// non-scaled frames.
  inline int getFrames(buffp buff, int frameCount) {
    if (note == sample->getNote()) {
      return getNormalFrames(buff, frameCount);
    }
    else {
      return getScaledFrames(buff, frameCount);
    }
  }

  inline void BEGIN() { while (__sync_lock_test_and_set(&stateLock, 1)); } ///< Lock a multithreading resource that needs to be modified
  inline void END()   { __sync_lock_release(&stateLock); } ///< Unlock a multithreading resource that has been modified

  inline bool isActive()   { return  active; }
  inline bool isInactive() { return !active; }

  inline bool isDormant()  { return state == DORMANT; }
  inline bool isOpening()  { return state == OPENING; }
  inline bool isAlive()    { return state == ALIVE;   }
  inline bool isClosing()  { return state == CLOSING; }

  inline void setState(voiceState value) { state = value; };

  inline void activate()   { if (isInactive()) setState(OPENING); active = true;  }
  inline void inactivate() { if (isActive())   setState(CLOSING); active = false; }

  inline void setNext(voicep n) { next = n; }
  inline voicep getNext() { return next; }
  inline char   getNote() { return note; }
  inline float  getGain() { return gain; }

  inline int getFadeOutPos() { return fadeOutPos; }
  inline bool isFadingOut()  { return fadingOut; }
  inline void incFadeOutPos(int value) { fadeOutPos += value; }

  /// Called by the OpenSample Thread to close a sample file when the
  /// state of the voice request so. Once closed, the state of the
  /// voice is changed to DORMANT.
  inline void closeSoundFile() { 
    BEGIN();
      if (soundFileOpen) sf_close(soundFile); 
      soundFileOpen = false;
      setState(DORMANT);
      clearFifo();
    END();
  }

  /// Called by the OpenSample Thread to open a sample file when the
  /// state of the voice request so. Once opened, the state of the
  /// voice is set to ALIVE.
  inline void openSoundFile() { 
    if (!soundFileOpen) {
      SF_INFO sfinfo;
      sfinfo.format = 0;
      if ((soundFile = sf_open(sample->getFilename(), SFM_READ, &sfinfo)) != NULL) {
	int loc = sf_seek(soundFile, sample->getFrameCount() + 1, SEEK_SET);
	if (loc != (sample->getFrameCount() + 1)) {
	  logger.DEBUG("Unable to seek...");
	}
	else {
	  soundFileOpen = true;
	  BEGIN();
	    setState(ALIVE);
	  END();
	}
      }
    }
  }

  /// This method is used by the SampleFeeder thread to read new data
  /// from the sample file and put it in the next avail slot in the
  /// fifo buffer, if there is some room available.
  inline void feedFifo() {
    if (isActive() && isAlive()) {
      if (!fifo->isFull()) {
	if (!soundFileOpen) {
	  logger.DEBUG("INTERNAL ERROR: sound file expected open.");
	}
	else {
	  BEGIN();
	    if (soundFileOpen) { // Just in case the other thread got control to close the file
	      fifo->setFrameCount(sf_readf_float(soundFile, 
						 fifo->getTail(), 
						 BUFFER_FRAME_COUNT));
	      fifo->push();
	    }
	  END();
	}
      }
    }
  }

  inline void clearFifo()       { fifo->clear();        }
  inline bool isSoundFileOPen() { return soundFileOpen; }
  inline void fadeOut()         { fadingOut = true;     }
  inline void noteOff()         { noteIsOn = false;     }
  inline bool isNoteOn()        { return noteIsOn;      }
};

#endif
