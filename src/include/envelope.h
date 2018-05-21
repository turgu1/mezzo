#ifndef _ENVELOPE_
#define _ENVELOPE_

#include <iostream>
#include <iomanip>

#include "mezzo.h"
#include "utils.h"

class Envelope
{
private:
  static bool allActive;

  uint32_t delay;
  uint32_t attack;
  uint32_t hold;
  uint32_t decay;
  uint32_t release;

  uint32_t attackStart;
  uint32_t holdStart;
  uint32_t decayStart;
  uint32_t sustainStart;

  int16_t  keynumToHold;
  int16_t  keynumToDecay;

  float attackRate;
  float decayRate;
  float releaseRate;

  float amplitude;
  float sustain;
  float attenuation;

  bool     keyReleased;
  uint32_t keyReleasedPos;

public:

  Envelope() 
  { 
    delay = attack = hold = decay = 0;
    keynumToHold = keynumToDecay = 0;
    amplitude = sustain = attenuation = 0.7f;
    release = centsToSampleCount(-3600); 
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive() { return allActive; }

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

  inline void setKeynumToHold  (int16_t k) { keynumToHold  = k; }
  inline void addToKeynumToHold(int16_t k) { keynumToHold += k; }

  inline void setKeynumToDecay  (int16_t k) { keynumToDecay  = k; }
  inline void addToKeynumToDecay(int16_t k) { keynumToDecay += k; }

  inline void setAttenuation  (int16_t a) { attenuation  = centibelToRatio(- a); }
  inline void addToAttenuation(int16_t a) { attenuation *= centibelToRatio(- a); }

  inline void setSustain(uint32_t s) 
  { 
    if (s >= 1000) {
      sustain = 0.0f;
    }
    else if (s <= 0) {
      sustain = 0.7f;
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

  void setup(uint8_t note) 
  {
    keyReleased   = false;

    attackStart   = delay;
    holdStart     = attackStart + attack;
    decayStart    = holdStart   + ((keynumToHold  == 0) ? 
                                    hold : 
                                    (centsToSampleCount(keynumToHold  * (60.0f - note))));
    sustainStart  = decayStart  + ((keynumToDecay == 0) ?
                                    decay : 
                                    (centsToSampleCount(keynumToDecay * (60.0f - note))));

    attackRate    = attack  == 0 ? attenuation : 
                                   (attenuation / (float) attack);

    decayRate     = decay   == 0 ? attenuation : 
                                   ((attenuation - sustain) / (float) decay);

    releaseRate   = release == 0 ? attenuation : 
                                   (sustain / (float) release);

    // showStatus();
  }

  inline bool keyIsReleased() { return keyReleased; }

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope.
  inline bool keyHasBeenReleased(uint32_t pos) 
  {
    if (!allActive) return true; // This will fake the end of the sound

    keyReleased    = true;
    keyReleasedPos = pos;
    releaseRate    = release == 0 ? 1.0f : 
                                    (amplitude / (float)release);
    return false;
  }

  inline bool transform(buffp src, uint16_t length, uint32_t pos) 
  {
    using namespace std;

    if (!allActive) return false; // Fake this it is not the end of the sound

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

  void showStatus(int spaces) 
  {
    using namespace std;
    cout 
      << setw(spaces) << ' ' << "Envelope: " << (allActive ? "Active" : "Inactive")
      << " [Delay:"   << delay
      << " Attack:"   << attack  << " rate="  << attackRate << " start=" << attackStart
      << " Hold:"     << hold    << " start=" << holdStart
      << " Decay:"    << decay   << " rate="  << decayRate  << " start=" << decayStart
      << " Sustain:"  << sustain << " start=" << sustainStart
      << " Release:"  << release << " rate="  << releaseRate
      << "] Att:"     << fixed   << setw(7)   << setprecision(5) << attenuation << endl;
  }
};

#endif