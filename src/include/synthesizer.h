#ifndef _SYNTHESIZER_
#define _SYNTHESIZER_

#include <iostream>

#include "vibrato.h"
#include "envelope.h"
#include "biquad.h"

// A synthesizer is containing the generators and code to
// transform samples in relashionship with the generators. A synthesizer
// is attached to each voice and is central to the transformation of
// sounds in the way the soundfont designer wanted it to be ear.

class Synthesizer {

private:
  Vibrato  vib;
  Envelope volEnvelope;
  BiQuad   biQuad;

  uint32_t pos;
  uint32_t start;
  uint32_t end;
  uint32_t startLoop;
  uint32_t endLoop;
  uint32_t sampleRate;
  uint32_t sizeSample;
  uint32_t sizeLoop;
  float    correctionFactor;
  int16_t  pan;
  int16_t  fineTune;
  uint8_t  rootKey;
  int8_t   keynum;
  int8_t   transpose;
  int8_t   velocity;
  bool     loop;

  enum setGensType { set, adjust, init };
  void setGens(sfGenList * gens, uint8_t genCount, setGensType type);

  // Two biQuad IIR parameters have been used:
  // 1. The biQuad parameters computation as a by-product of the following:
  //
  //   http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
  //
  // 2. The Butterworth parameters computation comes from the book "The Audio
  // Programming Book" by Richard Boulanger and Al., table 6.1
  //
  // Not sure if the end-state of it is the right one. Seems to be
  // similar to the one used in the Polyphone program.

  inline void toStereo(buffp dst, buffp src, uint16_t length) 
  {
    float fpan = pan / 1000.0f;

    const float prop  = M_SQRT2 * 0.5;
    const float angle = ((float) fpan) * M_PI;

    const float left  = prop * (cos(angle) - sin(angle));
    const float right = prop * (cos(angle) + sin(angle));

    if      (left  < 0.001) while (length--) { *dst++ = *src++; *dst++ = 0.0; }
    else if (right < 0.001) while (length--) { *dst++ = 0.0; *dst++ = *src++; }
    else {
      //std::cout << "OUPS!!!! : " << pan << "," << left << ", " << right << std::endl;
      while (length--) { *dst++ = *src * right; *dst++ = *src++ * left; }
    }
  }

public:
  inline void initGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, init);
  }
  inline void setGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, set);
  }
  inline void addGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, adjust);
  }
  void setDefaults(Sample * sample);

  void process(buffp buff);

  void showStatus(int spaces);
  void completeParams(uint8_t note);

  inline uint32_t getStart()       { return start;             }
  inline uint32_t getEnd()         { return end;               }
  inline uint32_t getStartLoop()   { return startLoop;         }
  inline uint32_t getEndLoop()     { return endLoop;           }
  inline uint32_t getSampleRate()  { return sampleRate;        }
  inline bool     isLooping()      { return loop;              }
  inline uint16_t getPan()         { return pan;               }
  inline uint32_t getSizeSample()  { return sizeSample;        }
  inline uint32_t getSizeLoop()    { return sizeLoop;          }
  inline float    getCorrection()  { return correctionFactor;  }
  inline uint8_t  getRootKey()     { return rootKey;           }
  inline int8_t   getVelocity()    { return velocity;          }
  inline int8_t   getKeynum()      { return keynum;            }
  inline int8_t   getTranspose()   { return transpose;         }
  inline int16_t  getFineTune()    { return fineTune;          }

  /// Returns true if this call must be considered the end of the note (in the
  /// case where the envelope as been desactivated)
  inline bool keyHasBeenReleased() { return volEnvelope.keyHasBeenReleased(); }

  bool transform(buffp dst, buffp src, uint16_t length);

  inline float vibrato(uint32_t pos) { return vib.nextValue(pos); }

  static bool areAllFilterActive()   { return BiQuad::areAllActive();   }
  static bool areAllVibratoActive()  { return Vibrato::areAllActive();  }
  static bool areAllEnvelopeActive() { return Envelope::areAllActive(); }

  static bool toggleFilter()     { return BiQuad::toggleAllActive();   }
  static bool toggleVibrato()    { return Vibrato::toggleAllActive();  }
  static bool toggleEnvelope()   { return Envelope::toggleAllActive(); }
};

#endif
