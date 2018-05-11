#ifndef _VIBRATO_
#define _VIBRATO_

#include "mezzo.h"
#include "lfo.h"
#include "utils.h"

class Vibrato
{
private:
  Lfo      lfo;
  float    pitch;
  float    frequency;
  uint32_t delay;
  float    pitchSamples;

public:
  Vibrato() { pitch = 0.0f; frequency = 1.0f; delay = 0; }

  inline void setup(uint8_t note) {
    lfo = Lfo(frequency, 3.0f * M_PI / 2.0f);
    pitchSamples = log2(noteFrequency(note) * pitch / 100.0f);
    showStatus();
  }

  inline void setPitch(int16_t p)     { pitch      = ((float) p); }
  inline void addToPitch(int16_t p)   { pitch     += ((float) p); }

  inline void setDelay(int16_t d)     { delay      = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDelay(int16_t d)   { delay      = (d == -32768) ? 0 : centsToSampleCount(d); }

  inline void setFrequency(float f)   { frequency  = centsToFreq(f); }
  inline void addToFrequency(float f) { frequency += centsToFreq(f); }

  inline float nextValue(uint32_t pos)
  {
    if ((pitch  == 0.0f) || (frequency == 0.0f)) {
      return 1.0f;
    }
    else {
      return (pos < delay) ? 0.0f : (pitchSamples + (lfo.nextValue() * pitchSamples));
    }
  }

  void showStatus()
  {
    using namespace std;

    cout <<"Vib:[D:" << delay
         << " P:"    << pitch
         << " F:"    << frequency
         << "] ";

    lfo.showStatus();
  }

};

#endif