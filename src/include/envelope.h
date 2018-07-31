#ifndef _ENVELOPE_
#define _ENVELOPE_

#include <iostream>
#include <iomanip>

#include "globals.h"
#include "mezzo.h"
#include "utils.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

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
    return 1.0f + (log(endLevel) - log(startLevel)) / (lengthInSamples);
  }

  inline void setup(uint8_t note) 
  {
    (void) note;

    keyReleased   = false;

    attackRate    = attack == 0 ? (attenuation) : 
                                  computeRate(0.0001f, attenuation, attack);
    decayRate     = decay  == 0 ? (attenuation) : 
                                  computeRate(attenuation, attenuation * sustain, decay);
    rate = 0;
  }

  inline bool keyIsReleased() { return keyReleased; }

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope. A quick release means a shortened
  /// period to go to a 0 amplitude. If the envelope is inactive (as requested
  /// by the user)
  inline bool keyHasBeenReleased(bool quick = false) 
  {
    if (!allActive) return true; // This will fake the end of the sound

    release     = quick ? 8000 : release;
    releaseRate = release == 0 ? 0.0f : computeRate(amplitude, 0.0001f, release);
    keyReleased = true;

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

  // From where we are in the stream of envelope levels, build an
  // array with the changes of amplitude to apply to a packet of
  // samples. Returns true if at the end of the envelope.
  //
  // If using NEON Intrinsics, length must be a multiple of 4.
  inline bool getAmplitudes(sampleRecord & amps, uint16_t length) 
  {
    if (!allActive) return false; // Fake this it is not the end of the sound

    if (state >= OFF) return true;

    if (keyReleased && (state < RELEASE)) {
      state = release == 0 ? OFF : RELEASE;
      rate  = releaseRate;
      ticks = release;
      
      if (state >= OFF) return true;
    }

    #if USE_NEON_INTRINSICS
      assert(((length & 0x03) == 0) && (length >= 4));

      const float zero = 0.0f;

      float rates[4];
      rates[0] = rate;
      rates[1] = rate * rate;
      rates[2] = rates[1] * rate;
      rates[3] = rates[2] * rate;
      __builtin_prefetch(rates);
      float32x4_t zeros = vld1q_dup_f32(&zero);
      float32x4_t attenuations = vld1q_dup_f32(&attenuation);
      float32x4_t rts = vld1q_f32(rates);

      for (int i = 0; i < length; i += 4) {
        float32x4_t amplitudes = vld1q_dup_f32(&amplitude);
        amplitudes = vmulq_f32(amplitudes, rts);
        amplitudes = vminq_f32(amplitudes, attenuations);
        amplitudes = vmaxq_f32(amplitudes, zeros);
        vst1q_f32(&amps[i], amplitudes);
        amplitude = amps[i + 3];
        if (ticks <= 4) {
          nextState();
          rates[0] = rate;
          rates[1] = rate * rate;
          rates[2] = rates[1] * rate;
          rates[3] = rates[2] * rate;
          __builtin_prefetch(rates);
          rts = vld1q_f32(rates);
        }
        else {
          ticks -= 4;
        }
      }
    #else
      for (int i = 0; i < length; i++) {
        if (ticks-- == 0) nextState();
        amplitude = MAX(MIN(attenuation, amplitude * rate), 0.0f);
        amps[i] = amplitude;
      }
    #endif

    return state == OFF;
  }

  void showStatus(int spaces) ;
};

#endif
