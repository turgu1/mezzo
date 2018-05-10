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

public:
  Vibrato() { pitch = 0.0f; frequency = 1.0f; delay = 0; setup(); }

  inline void setup() {
    lfo = Lfo(frequency, 3.0f * M_PI / 2.0f);
  }

  inline void setPitch(int16_t p)     { pitch      = ((float) p) / 200.0f; }
  inline void addToPitch(int16_t p)   { pitch     += ((float) p) / 200.0f; }

  inline void setDelay(int16_t d)     { delay      = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDelay(int16_t d)   { delay     += (d == -32768) ? 0 : centsToSampleCount(d); }

  inline void setFrequency(float f)   { frequency  = centsToFreq(f); }
  inline void addToFrequency(float f) { frequency += centsToFreq(f); }

  inline float nextValue(uint32_t pos)
  {
    if ((pitch  == 0.0f) || (frequency == 0.0f)) {
      return 1.0f;
    }
    else {
      if (pos < delay) {
        return 1.0f;
      }
      else {
        return pow(2, (pitch + (lfo.nextValue() * (pitch))) / 12);
      }
    }
  }

  void showStatus()
  {
    std::cout <<"Vib:[D:" << delay
              << ",P:"    << pitch
              << ",F:"    << frequency
              << "]"      << std::endl;
  }

};

#endif