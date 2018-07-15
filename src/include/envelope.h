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

  enum State {
    START = 0, DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE, OFF
  };

  uint8_t  state;

  uint32_t delay;
  uint32_t attack;
  uint32_t hold;
  uint32_t decay;
  uint32_t release;

  int16_t  keynumToHold;
  int16_t  keynumToDecay;

  float    rate;
  uint32_t ticks;

  float attackRate;
  float decayRate;
  float releaseRate;

  float amplitude;
  float sustain;
  float attenuation;

  volatile bool keyReleased;

public:

  Envelope() 
  { 
    ticks  = 
    delay  = 
    attack = 
    hold   = 
    decay  = 0;

    keynumToHold  = 
    keynumToDecay = 0;

    amplitude = 0.0f;

    sustain     = 
    attenuation = 1.0f;
    
    release     = centsToSampleCount(-3600); 

    state = START;
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive() { return allActive; }

  inline void setDelay    (int16_t d) { delay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 5000, 0)); }
  inline void addToDelay  (int16_t d) { delay   *= (d == -32768) ? 1 : centsToRatio(Utils::checkRange(d, -12000, 5000, 0)); }

  inline void setAttack   (int16_t a) { attack   = (a == -32768) ? 0 : centsToSampleCount(Utils::checkRange(a, -12000, 8000, 0)); }
  inline void addToAttack (int16_t a) { attack  *= (a == -32768) ? 1 : centsToRatio(Utils::checkRange(a, -12000, 8000, 0)); }

  inline void setHold     (int16_t h) { hold     = (h == -32768) ? 0 : centsToSampleCount(Utils::checkRange(h, -12000, 5000, 0)); }
  inline void addToHold   (int16_t h) { hold    *= (h == -32768) ? 1 : centsToRatio(Utils::checkRange(h, -12000, 5000, 0)); }

  inline void setDecay    (int16_t d) { decay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 8000, 0)); }
  inline void addToDecay  (int16_t d) { decay   *= (d == -32768) ? 1 : centsToRatio(Utils::checkRange(d, -12000, 8000, 0)); }

  inline void setRelease  (int16_t r) { release  = (r == -32768) ? 0 : centsToSampleCount(Utils::checkRange(r, -12000, 8000, 0)); }
  inline void addToRelease(int16_t r) { release *= (r == -32768) ? 1 : centsToRatio(Utils::checkRange(r, -12000, 8000, 0)); }

  inline void setKeynumToHold  (int16_t k) { keynumToHold  = k; }
  inline void addToKeynumToHold(int16_t k) { keynumToHold += k; }

  inline void setKeynumToDecay  (int16_t k) { keynumToDecay  = k; }
  inline void addToKeynumToDecay(int16_t k) { keynumToDecay += k; }

  inline void setAttenuation  (int16_t a) { attenuation  = centibelToRatio(- (a >> 1)); }
  inline void addToAttenuation(int16_t a) { attenuation *= centibelToRatio(- (a >> 1)); }

  // This is the decrease in level, expressed in centibels, to which the Volume 
  // Envelope value ramps during the decay phase. For the Volume Envelope, the 
  // sustain level is best expressed in centibels of attenuation from full scale. 
  // A value of 0 indicates the sustain level is full level; this implies a zero 
  // duration of decay phase regardless of decay time. A positive value indicates 
  // a decay to the corresponding level. Values less than zero are to be 
  // interpreted as zero; conventionally 1000 indicates full attenuation. 
  // For example, a sustain level which corresponds to an absolute value 12dB 
  // below of peak would be 120.

  inline void setSustain(int32_t s) 
  {
    if (s >= 1000) {
      sustain = 1.0f;
    }
    else if (s <= 0) {
      sustain = 0.0f;
    }
    else {
      sustain = centibelToRatio(- s);
    }
  }

  inline void addToSustain(int32_t s)
  { 
    if (s >= 1000) {
      sustain = 0.1f;
    }
    else if (s <= 0) {
    }
    else {
      sustain *= centibelToRatio(- s);
    }
  }

  inline float computeRate(float startLevel, float endLevel, uint32_t lengthInSamples) 
  {
    float res = 1.0f + (log(endLevel) - log(startLevel)) / (lengthInSamples);

    //std::cout << "ComputeRate: Start:" << startLevel << ", End: " << endLevel << ", Length: " << lengthInSamples << std::endl;
    
    return res;
  }

  inline void setup(uint8_t note) 
  {
    (void) note;

    keyReleased   = false;

    // decayStart    = holdStart   + ((keynumToHold  == 0) ? 
    //                                 hold : 
    //                                 (centsToSampleCount(keynumToHold  * (60.0f - note))));
    // sustainStart  = decayStart  + ((keynumToDecay == 0) ?
    //                                 decay : 
    //                                 (centsToSampleCount(keynumToDecay * (60.0f - note))));

    // attackRate    = attack == 0 ? (1.0f - attenuation) : 
    //                               computeRate(0.0001f, 1.0f - attenuation, attack);
    // decayRate     = decay  == 0 ? (1.0f - attenuation) : 
    //                               computeRate(1.0f - attenuation, 1.0f - attenuation - sustain, decay);

    attackRate    = attack == 0 ? (attenuation) : 
                                  computeRate(0.0001f, attenuation, attack);
    decayRate     = decay  == 0 ? (attenuation) : 
                                  computeRate(attenuation, attenuation * sustain, decay);

    rate = 0;
    //std::cout << "Setup: Attack: " << attack << ", Rate: " << attackRate << ", Decay: " << decay << ", Rate: " << decayRate << std::endl;

    // showStatus();
  }

  inline bool keyIsReleased() { return keyReleased; }

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope.
  inline bool keyHasBeenReleased(bool quick = false) 
  {
    if (!allActive) return true; // This will fake the end of the sound

    releaseRate    = release == 0 ? 0.0f : computeRate(amplitude, 0.0001f, quick ? 8000 : release);
    keyReleased    = true;

    //std::cout << "KeyHasBeenReleased: Rate: " << releaseRate << ", Amplitude: " << amplitude << ", Release: " << release << std::endl;

    return false;
  }

  inline void nextState()
  {
    switch (++state) {
      case DELAY:
        if (delay > 0) {
          ticks     = delay;
          amplitude = 0.0f;
          rate      = 0.0f;
          break;
        }
        state     = ATTACK;

      case ATTACK:
        if (attack > 0) {
          amplitude = 0.0001f;
          ticks     = attack;
          rate      = attackRate;
          break;
        }
        state = HOLD;
        amplitude = attenuation;

      case HOLD:
        if (hold > 0) {
          ticks     = hold;
          amplitude = attenuation;
          rate      = 1.0f;
          break;
        }
        state = DECAY;

      case DECAY:
        if (decay > 0) {
          ticks     = decay;
          rate      = decayRate;
          break;
        }
        state = SUSTAIN;

      case SUSTAIN:
        ticks = 0xFFFFFFFFU;
        rate  = 1.0f;
        break;

      case RELEASE:
        break;

      default:
        state     = OFF;
        amplitude = 0.0f;
        rate      = 0.0f;
        break;
    }
  }

  inline bool transform(buffp src, uint16_t length) 
  {
    if (!allActive) return false; // Fake this it is not the end of the sound

    if (state >= OFF) return true;

    if (keyReleased && (state < RELEASE)) {
      state = release == 0 ? OFF : RELEASE;
      rate  = releaseRate;
      ticks = release;
      
      if (state >= OFF) return true;
    }


    // while (length--) {
    //   if (keyReleased) {                                         // key release
    //     if (pos < (keyReleasedPos + release)) {
    //       amplitude *= releaseRate;
    //     }
    //     else {
    //       amplitude  = 0.0f;
    //       endOfSound = true;
    //     }
    //   }
    //   else {
    //     if      (pos < attackStart ) amplitude  = 0.0001f;       // delay
    //     else if (pos < holdStart   ) amplitude *= attackRate;    // attack
    //     else if (pos < decayStart  ) ;                           // hold
    //     else if (pos < sustainStart) amplitude *= decayRate;     // decay
    //     else                         amplitude  = sustain - attenuation;       // sustain
    //   }
    //   amplitude = MAX(MIN(1.0f, amplitude), 0.0f);
    //   *src++ *= amplitude;
    //   pos += 1;
    // }

    while (length--) {
      if (ticks-- == 0) nextState();
      // switch (state) {
      //   case DELAY:
      //     if (delay > 0) {
      //       delay --;
      //       amplitude = 0.0f;
      //       break;
      //     }
      //     state     = ATTACK;
      //     amplitude = 0.0001f;

      //   case ATTACK:
      //     if (attack > 0) {
      //       attack--;
      //       amplitude *= attackRate;
      //       break;
      //     }
      //     state = HOLD;
      //     amplitude = attenuation;

      //   case HOLD:
      //     if (hold > 0) {
      //       hold --;
      //       break;
      //     }
      //     state = DECAY;

      //   case DECAY:
      //     if (decay > 0) {
      //       decay --;
      //       amplitude *= decayRate;
      //       break;
      //     }
      //     state = SUSTAIN;

      //   case SUSTAIN:
      //     break;

      //   case RELEASE:
      //     if (release > 0) {
      //       release --;
      //       amplitude *= releaseRate;
      //       break;
      //     }
      //     state = OFF;
      //     amplitude = 0.0f;

      //   case OFF:
      //     endOfSound = true;
      //     break;
      // }

      amplitude *= rate;
      amplitude = MAX(MIN(attenuation, amplitude), 0.0f);
      *src++ *= amplitude;
    }

    // std::cout << amplitude << ", ";
    //std::cout << "(St:" << +state << ", Ampl:" << amplitude << ", Tcks:" << ticks << ", Att:" << attenuation << ")" << std::endl;

    return state == OFF;
  }

  void showStatus(int spaces) ;
};

#endif
