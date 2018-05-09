#include "mezzo.h"

#include <cmath>

Lfo::Lfo()
{
  frequency = 0.0f;
  phase     = 0.0f;
  bitRate   = 44100;
}

Lfo::Lfo(float frequency, float phase, uint16_t bitRate)
{
  this->frequency = frequency;
  this->phase = phase;
  this->bitRate = bitRate;

  restart();
}

void Lfo::restart()
{
  increment = (frequency * 2 * M_PI) / bitRate;
  currentPhase = phase;
}

float Lfo::nextValue()
{
  if (frequency == 0.0f) return 0.0f;

  float value = sin(currentPhase);
  currentPhase += increment;
  if (currentPhase >= (M_PI * 2)) currentPhase -= (M_PI * 2);

  return value;
}
