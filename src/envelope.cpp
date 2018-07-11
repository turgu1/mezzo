#include "mezzo.h"

#include "envelope.h"

bool Envelope::allActive = true;

  bool Envelope::toggleAllActive() { return allActive = !allActive; }
  bool Envelope::areAllActive() { return allActive; }

  void Envelope::setDelay    (int16_t d) { delay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 5000, 0)); }
  void Envelope::addToDelay  (int16_t d) { delay   *= (d == -32768) ? 1 : centsToRatio(Utils::checkRange(d, -12000, 5000, 0)); }

  void Envelope::setAttack   (int16_t a) { attack   = (a == -32768) ? 0 : centsToSampleCount(Utils::checkRange(a, -12000, 8000, 0)); }
  void Envelope::addToAttack (int16_t a) { attack  *= (a == -32768) ? 1 : centsToRatio(Utils::checkRange(a, -12000, 8000, 0)); }

  void Envelope::setHold     (int16_t h) { hold     = (h == -32768) ? 0 : centsToSampleCount(Utils::checkRange(h, -12000, 5000, 0)); }
  void Envelope::addToHold   (int16_t h) { hold    *= (h == -32768) ? 1 : centsToRatio(Utils::checkRange(h, -12000, 5000, 0)); }

  void Envelope::setDecay    (int16_t d) { decay    = (d == -32768) ? 0 : centsToSampleCount(Utils::checkRange(d, -12000, 8000, 0)); }
  void Envelope::addToDecay  (int16_t d) { decay   *= (d == -32768) ? 1 : centsToRatio(Utils::checkRange(d, -12000, 8000, 0)); }

  void Envelope::setRelease  (int16_t r) { release  = (r == -32768) ? 0 : centsToSampleCount(Utils::checkRange(r, -12000, 8000, 0)); }
  void Envelope::addToRelease(int16_t r) { release *= (r == -32768) ? 1 : centsToRatio(Utils::checkRange(r, -12000, 8000, 0)); }

  void Envelope::setKeynumToHold  (int16_t k) { keynumToHold  = k; }
  void Envelope::addToKeynumToHold(int16_t k) { keynumToHold += k; }

  void Envelope::setKeynumToDecay  (int16_t k) { keynumToDecay  = k; }
  void Envelope::addToKeynumToDecay(int16_t k) { keynumToDecay += k; }

  void Envelope::setAttenuation  (int16_t a) { attenuation  = centibelToRatio(- (a >> 1)); }
  void Envelope::addToAttenuation(int16_t a) { attenuation *= centibelToRatio(- (a >> 1)); }

  // This is the decrease in level, expressed in centibels, to which the Volume 
  // Envelope value ramps during the decay phase. For the Volume Envelope, the 
  // sustain level is best expressed in centibels of attenuation from full scale. 
  // A value of 0 indicates the sustain level is full level; this implies a zero 
  // duration of decay phase regardless of decay time. A positive value indicates 
  // a decay to the corresponding level. Values less than zero are to be 
  // interpreted as zero; conventionally 1000 indicates full attenuation. 
  // For example, a sustain level which corresponds to an absolute value 12dB 
  // below of peak would be 120.

  void Envelope::setSustain(int32_t s) 
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

  void Envelope::addToSustain(int32_t s)
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

  void Envelope::setup(uint8_t note) 
  {
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
    //std::cout << "Setup: Attack: " << attack << ", Rate: " << attackRate << ", Decay: " << decay << ", Rate: " << decayRate << std::endl;

    // showStatus();
  }

  bool Envelope::keyIsReleased() { return keyReleased; }

  /// When the key has been released by the player, prepare for the
  /// release portion of the envelope.
  bool Envelope::keyHasBeenReleased(bool quick) 
  {
    if (!allActive) return true; // This will fake the end of the sound

    keyReleased    = true;
    releaseRate    = release == 0 ? 0.0f : computeRate(amplitude, 0.0001f, quick ? 8000 : release);

    //std::cout << "KeyHasBeenReleased: Rate: " << releaseRate << ", Amplitude: " << amplitude << ", Release: " << release << std::endl;

    return false;
  }

  bool Envelope::transform(buffp src, uint16_t length) 
  {
    if (!allActive) return false; // Fake this it is not the end of the sound

    bool endOfSound = false;

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

    if (keyReleased) state = RELEASE;

    while (length--) {
      switch (state) {
        case DELAY:
          if (delay > 0) {
            delay --;
            amplitude = 0.0f;
            break;
          }
          state     = ATTACK;
          amplitude = 0.0001f;

        case ATTACK:
          if (attack > 0) {
            attack--;
            amplitude *= attackRate;
            break;
          }
          state = HOLD;
          amplitude = attenuation;

        case HOLD:
          if (hold > 0) {
            hold --;
            break;
          }
          state = DECAY;

        case DECAY:
          if (decay > 0) {
            decay --;
            amplitude *= decayRate;
            break;
          }
          state = SUSTAIN;

        case SUSTAIN:
          break;

        case RELEASE:
          if (release > 0) {
            release --;
            amplitude *= releaseRate;
            break;
          }
          state = OFF;
          amplitude = 0.0f;

        case OFF:
          endOfSound = true;
          break;
      }
      
      amplitude = MAX(MIN(attenuation, amplitude), 0.0f);
      *src++ *= amplitude;
    }

    // std::cout << amplitude << ", ";

    return endOfSound;
  }

  void Envelope::showStatus(int spaces) 
  {
    using namespace std;
    cout 
      << setw(spaces) << ' ' << "Envelope: " << (allActive ? "Active" : "Inactive")
      << " [Delay:"   << delay
      << " Attack:"   << attack  << "(rate="  << attackRate << ")"
      << " Hold:"     << hold 
      << " Decay:"    << decay   << "(rate="  << decayRate  << ")"
      << " Sustain:"  << sustain 
      << " Release:"  << release << "(rate="  << releaseRate  << ")"
      << "] Att:"     << fixed   << setw(7)   << setprecision(5) << attenuation << endl;
  }