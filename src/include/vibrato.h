#ifndef _VIBRATO_
#define _VIBRATO_

#include <iomanip>

#include "mezzo.h"
#include "lfo.h"
#include "utils.h"

class Vibrato
{
private:
  static bool allActive;

  Lfo      lfo;
  float    pitch;
  float    frequency;
  uint32_t delay;
  float    pitchSamples;

public:
  Vibrato() { pitch = 0.0f; frequency = 8.176f; delay = centsToSampleCount(-12000); }

  inline void setup(uint8_t note) {
    // The chosen phase is to get a smooth transition at the beginning of the vibrato integration.
    lfo = Lfo(frequency, 3.0f * M_PI / 2.0f);
    pitchSamples = log2(noteFrequency(note) * pitch / 100.0f);
    // showStatus();
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive()    { return allActive;              }

  inline void setPitch(int16_t p)     { pitch      = ((float) p); }
  inline void addToPitch(int16_t p)   { pitch     += ((float) p); }

  inline void setDelay(int16_t d)     { delay      = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDelay(int16_t d)   { delay     *= (d == -32768) ? 1 : centsToRatio(d); }

  inline void setFrequency(float f)   { frequency  = centsToFreq(f); }
  inline void addToFrequency(float f) { frequency *= centsToRatio(f); }

  inline float nextValue(uint32_t pos)
  {
    if (allActive && (pitch  != 0.0f) && (frequency != 0.0f)) {
      return (pos < delay) ? 0.0f : (pitchSamples + (lfo.nextValue() * pitchSamples));
    }
    else {
      return 0.0f;
    }
  }

  void showStatus(int spaces)
  {
    using namespace std;

    cout << setw(spaces) << ' '
         <<"Vibrato: " << ((allActive && (pitch  != 0.0f) && (frequency != 0.0f)) ? "Active" : "Inactive")
         << " [Delay:" << delay
         << " Pitch:"  << pitch
         << " Freq:"   << frequency
         << "] ";

    lfo.showStatus(2);
  }

};

#endif