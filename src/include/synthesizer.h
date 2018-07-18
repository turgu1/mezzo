#ifndef _SYNTHESIZER_
#define _SYNTHESIZER_

#include <iostream>

#include "globals.h"
#include "vibrato.h"
#include "envelope.h"
#include "biquad.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

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
  float32_t left, right;
  int16_t  pan;
  int16_t  fineTune;
  uint8_t  rootKey;
  int8_t   keynum;
  int8_t   transpose;
  int8_t   velocity;
  bool     loop;

  enum setGensType { set, adjust, init };
  void setGens(sfGenList * gens, uint8_t genCount, setGensType type);

  inline void toStereoAndAdd(buffp dst, buffp src, buffp env, uint16_t length, float32_t gain) 
  {
    #if USE_NEON_INTRINSICS
      buffp s = &src[length];
      while (length & 0x03) {
        *s++ = 0.0f;
        length++;
      }
      assert((length >= 4) && (length <= SAMPLE_BUFFER_SAMPLE_COUNT));
    #else
      assert((length >= 1) && (length <= SAMPLE_BUFFER_SAMPLE_COUNT));
    #endif


    if (pan >=  250) {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;
        float32x4_t   envData;

        int count = length >> 2;
        while (count--) {
          __builtin_prefetch(env);
          __builtin_prefetch(dst);
          __builtin_prefetch(src);
          envData = vld1q_f32(env);
          dstData = vld2q_f32(dst);
          srcData = vld1q_f32(src);
          envData = vmulq_n_f32(envData, gain);
          dstData.val[0] = vmlaq_f32(dstData.val[0], srcData, envData);
          vst2q_f32(dst, dstData);
          src += 4;
          env += 4;
          dst += 8;
        }
      #else
        while (length--) { *dst += (*src++ * gain * *env++); dst += 2; }
      #endif
    }
    else if (pan <= -250) {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;
        float32x4_t   envData;

        int count = length >> 2;
        while (count--) {
          __builtin_prefetch(env);
          __builtin_prefetch(dst);
          __builtin_prefetch(src);
          envData = vld1q_f32(env);
          dstData = vld2q_f32(dst);
          srcData = vld1q_f32(src);
          envData = vmulq_n_f32(envData, gain);
          dstData.val[1] = vmlaq_f32(dstData.val[1], srcData, envData);
          vst2q_f32(dst, dstData);
          src += 4;
          env += 4;
          dst += 8;
        }
      #else
        dst++;
        while (length--) { *dst += (*src++ * gain * *env++); dst += 2; }
      #endif
    }
    else {
      #if USE_NEON_INTRINSICS
        float32x4x2_t dstData;
        float32x4_t   srcData;
        float32x4_t   leftFactor;
        float32x4_t   rightFactor;

        int count = length >> 2;
        while (count--) {
          __builtin_prefetch(env);
          __builtin_prefetch(dst);
          __builtin_prefetch(src);

          leftFactor  = vld1q_f32(env);
          rightFactor = vld1q_f32(env);
          dstData     = vld2q_f32(dst);
          srcData     = vld1q_f32(src);
          leftFactor  = vmulq_n_f32(leftFactor,  gain );
          rightFactor = vmulq_n_f32(rightFactor, gain );
          leftFactor  = vmulq_n_f32(leftFactor,  left );
          rightFactor = vmulq_n_f32(rightFactor, right);

          dstData.val[0] = vmlaq_f32(dstData.val[0], srcData, rightFactor);
          dstData.val[1] = vmlaq_f32(dstData.val[1], srcData, leftFactor);
          
          vst2q_f32(dst, dstData);
          
          src += 4;
          env += 4;
          dst += 8;
        }
      #else
        while (length--) { 
          *dst++ += *src * right * gain * *env; 
          *dst++ += *src++ * left * gain * *env++; 
        }
      #endif
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

  inline float vibrato(uint32_t pos) { return vib.nextValue(pos); }

  static bool areAllFilterActive()   { return BiQuad::areAllActive();   }
  static bool areAllVibratoActive()  { return Vibrato::areAllActive();  }
  static bool areAllEnvelopeActive() { return Envelope::areAllActive(); }

  static bool toggleFilter()     { return BiQuad::toggleAllActive();   }
  static bool toggleVibrato()    { return Vibrato::toggleAllActive();  }
  static bool toggleEnvelope()   { return Envelope::toggleAllActive(); }

  inline bool transformAndAdd(buffp dst, buffp src, uint16_t length, float32_t gain)
  {
    float env[SAMPLE_BUFFER_SAMPLE_COUNT];

    bool endOfSound;

    //biQuad.filter(src, length);

    assert(length <= SAMPLE_BUFFER_SAMPLE_COUNT);

    endOfSound = volEnvelope.transform(env, length);

    toStereoAndAdd(dst, src, env, length, gain);

    pos += length;

    // if (endOfSound) std::cout << "End of Sound" << std::endl;
    // std::cout << "[" << amplVolEnv << "]" << std::endl;

    return endOfSound;
  }
};

#endif
