#ifndef _ENVELOPE_
#define _ENVELOPE_

#include <iostream>
#include <iomanip>

#include "mezzo.h"
#include "utils.h"

class Envelope
{
private:
  uint32_t delay;
  uint32_t attack;
  uint32_t hold;
  uint32_t decay;
  uint32_t release;

  uint32_t attackStart;
  uint32_t holdStart;
  uint32_t decayStart;
  uint32_t sustainStart;

  uint32_t attackRate;
  uint32_t decayRate;
  uint32_t releaseRate;

  float amplitude;
  float sustain;
  float attenuation;

  bool     keyReleased;
  uint32_t keyReleasedPos;

public:

  Envelope() 
  { 
    delay = attack = hold = decay = 0;
    release = centsToSampleCount(-3600); 
    amplitude = sustain = attenuation = 1.0f;
    setup();
  }

  inline void setDelay    (int16_t d) { delay    = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDelay  (int16_t d) { delay   += (d == -32768) ? 0 : centsToSampleCount(d); }

  inline void setAttack   (int16_t a) { attack   = (a == -32768) ? 0 : centsToSampleCount(a); }
  inline void addToAttack (int16_t a) { attack  += (a == -32768) ? 0 : centsToSampleCount(a); }

  inline void setHold     (int16_t h) { hold     = (h == -32768) ? 0 : centsToSampleCount(h); }
  inline void addToHold   (int16_t h) { hold    += (h == -32768) ? 0 : centsToSampleCount(h); }

  inline void setDecay    (int16_t d) { decay    = (d == -32768) ? 0 : centsToSampleCount(d); }
  inline void addToDecay  (int16_t d) { decay   += (d == -32768) ? 0 : centsToSampleCount(d); }

  inline void setRelease  (int16_t r) { release  = (r == -32768) ? 0 : centsToSampleCount(r); }
  inline void addToRelease(int16_t r) { release += (r == -32768) ? 0 : centsToSampleCount(r); }

  inline void setAttenuation  (int16_t a) { attenuation  = centibelToRatio(- a); }
  inline void addToAttenuation(int16_t a) { attenuation *= centibelToRatio(- a); }

  inline void setSustain(uint32_t s) 
  { 
    if (s >= 1000) {
      sustain = 0.0f;
    }
    else if (s <= 0) {
      sustain = 1.0f;
    }
    else {
      sustain = centibelToRatio(- s);
    }
  }

  inline void addToSustain(uint32_t s)
  { 
    if (s >= 1000) {
    }
    else if (s <= 0) {
      sustain = 0.0f;
    }
    else {
      sustain *= centibelToRatio(- s);
    }
  }

  void setup() 
  {
    keyReleased   = false;

    attackStart   = delay;
    holdStart     = attackStart  + attack;
    decayStart    = holdStart    + hold;
    sustainStart  = decayStart   + decay;

    attackRate    = attack  == 0 ? attenuation : 
                                   (attenuation / (float) attack);

    decayRate     = decay   == 0 ? attenuation : 
                                   ((attenuation - sustain) / (float) decay);

    releaseRate   = release == 0 ? attenuation : 
                                   (sustain / (float) release);
  }

  inline bool keyIsReleased() { return keyReleased; }

  inline void keyHasBeenReleased(uint32_t pos) 
  {
    keyReleased    = true;
    keyReleasedPos = pos;
    releaseRate    = release == 0 ? 1.0f : 
                                    (amplitude / (float)release);
  }

  inline bool transform(buffp src, uint16_t length, uint32_t pos) 
  {
    using namespace std;

    bool endOfSound = false;

    while (length--) {
      if (keyReleased) {                                         // key release
        if (pos < (keyReleasedPos + release)) {
          amplitude -= releaseRate;
        }
        else {
          amplitude  = 0.0f;
          endOfSound = true;
        }
      }
      else {
        if      (pos < attackStart ) amplitude  = 0.0;           // delay
        else if (pos < holdStart   ) amplitude += attackRate;    // attack
        else if (pos < decayStart  ) ;                           // hold
        else if (pos < sustainStart) amplitude -= decayRate;     // decay
        else                         amplitude  = sustain;       // sustain
      }
      amplitude = MAX(MIN(attenuation, amplitude), 0.0f);
      *src++ *= amplitude;
      pos += 1;
    }

    return endOfSound;
  }

  void showStatus() 
  {
    using namespace std;
    cout 
      << ":[D:" << delay
      << ",A:"  << attack  << "@" << attackRate << "/" << attackStart
      << ",H:"  << hold    << "/" << holdStart
      << ",D:"  << decay   << "@" << decayRate  << "/" << decayStart
      << ",S:"  << sustain << "/" << sustainStart
      << ",R:"  << release << "@" << releaseRate
      << "] attenuation:" << fixed << setw(7) << setprecision(5) << attenuation << endl;
  }
};

#endif