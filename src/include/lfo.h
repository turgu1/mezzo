#ifndef _LFO_
#define _LFO_

class Lfo
{
private:
  float    frequency;
  float    phase;
  float    currentPhase;
  float    increment;
  uint16_t bitRate;


public:
  Lfo();
  Lfo(float frequency, float phase, uint16_t bitRate);

  inline void setFrequency(float _frequency) { this->frequency = _frequency; }
  inline void setPhase(float _phase) { this->phase = _phase; }
  inline void setBitRate(float _bitRate) { this->bitRate = _bitRate; }

  void  restart();
  float nextValue();
};

#endif
