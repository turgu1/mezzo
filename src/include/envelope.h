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
    DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE, OFF
  };

  uint8_t  state;

  uint32_t delay;
  uint32_t attack;
  uint32_t hold;
  uint32_t decay;
  uint32_t release;

  int16_t  keynumToHold;
  int16_t  keynumToDecay;

  float attackRate;
  float decayRate;
  float releaseRate;

  float amplitude;
  float sustain;
  float attenuation;

  bool     keyReleased;

public:

  Envelope() 
  { 
    delay  = 
    attack = 
    hold   = 
    decay  = 0;

    keynumToHold  = 
    keynumToDecay = 0;

    amplitude = 1.0f;

    sustain     = 
    attenuation = 1.0f;
    
    release     = centsToSampleCount(-3600); 

    state = DELAY;
  }

  static bool toggleAllActive();
  static bool areAllActive();

  void setDelay    (int16_t d);
  void addToDelay  (int16_t d);

  void setAttack   (int16_t a);
  void addToAttack (int16_t a);

  void setHold     (int16_t h);
  void addToHold   (int16_t h);

  void setDecay    (int16_t d);
  void addToDecay  (int16_t d);

  void setRelease  (int16_t r);
  void addToRelease(int16_t r);

  void setKeynumToHold  (int16_t k);
  void addToKeynumToHold(int16_t k);

  void setKeynumToDecay  (int16_t k);
  void addToKeynumToDecay(int16_t k);

  void setAttenuation  (int16_t a);
  void addToAttenuation(int16_t a);

  // This is the decrease in level, expressed in centibels, to which the Volume 
  // Envelope value ramps during the decay phase. For the Volume Envelope, the 
  // sustain level is best expressed in centibels of attenuation from full scale. 
  // A value of 0 indicates the sustain level is full level; this implies a zero 
  // duration of decay phase regardless of decay time. A positive value indicates 
  // a decay to the corresponding level. Values less than zero are to be 
  // interpreted as zero; conventionally 1000 indicates full attenuation. 
  // For example, a sustain level which corresponds to an absolute value 12dB 
  // below of peak would be 120.

  void setSustain(int32_t s);

  void addToSustain(int32_t s);

  void setup(uint8_t note);

  bool keyIsReleased();

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope.
  bool keyHasBeenReleased(bool quick = false);

  bool transform(buffp src, uint16_t length);
  void showStatus(int spaces) ;
};

#endif