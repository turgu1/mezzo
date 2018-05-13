#ifndef _LFO_
#define _LFO_

#include <cmath>
#include <iomanip>

#include "mezzo.h"

class Lfo
{
private:
  float    frequency;
  float    phase;
  float    currentPhase;
  float    increment;

  void setup() { increment    = (frequency * 2 * M_PI) / config.samplingRate;
                 currentPhase = phase; }

public:
  Lfo() { frequency = 0.0f; phase = 0.0f; }
  
  Lfo(float _frequency, float _phase) { frequency = _frequency; 
                                        phase     = _phase; 
                                        setup(); }

  inline void setFrequency(float f) { frequency = f; }
  inline void setPhase    (float p) { phase     = p; }

  float nextValue()
  {
    if (frequency == 0.0f) return 0.0f;

    float value = sin(currentPhase);
    currentPhase += increment;
    if (currentPhase >= (M_PI * 2)) currentPhase -= (M_PI * 2);

    assert((value >= -1.0f) && (value <= 1.0f));
    return value;
  }

  void showStatus(int spaces)
  {
    using namespace std;

    cout
      << setw(spaces) << ' '
      << "Lfo:"
      << "[Freq:"      << frequency
      << " Phase:"     << phase
      << " CurrPhase:" << currentPhase
      << " Incr:"      << increment
      << "]" << endl;
  }
};

#endif
