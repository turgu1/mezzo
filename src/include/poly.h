#include "copyright.h"

#ifndef _POLY_
#define _POLY_

#include "mezzo.h"

class Poly : public NewHandlerSupport<Poly> {

 private:
  voicep       voices;
  volatile int voiceCount;
  volatile int maxVoiceCount;

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

 public:
   Poly();
  ~Poly();

  int mixer(frameRecord & buff);

  void   inactivateAllVoices();
  void   showState();
  voicep firstVoice();
  voicep nextVoice(voicep prev);
  void   addVoice(samplep s, uint8_t note, float gain, Synthesizer & synth,
                  Preset & preset, uint16_t presetZoneIdx);

  voicep nextAvailable();
  void   noteOff(char note, bool pedal_on);
  void   voicesSustainOff();
  voicep removeVoice(voicep v, voicep prev);
  //int    getFrames(voicep v, buffp buff, int count);
  void   monitorCount();

  inline voicep getVoices() { return voices; }
  inline void   decVoiceCount() { voiceCount--; }
  inline int    getVoiceCount() { return voiceCount; }

  bool showVoicePlayingState() { return Voice::togglePlayingState(); };
};

void   stopThreads();
void * samplesFeeder(void * args);
void * voicesFeeder1(void * args);
void * voicesFeeder2(void * args);

#endif // POLY_H
