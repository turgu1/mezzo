#include "copyright.h"

#include "globals.h"

#if USE_NEON_INTRINSICS
  #include <arm_neon.h>
#endif

#ifndef _UTILS_
#define _UTILS_

#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#define MAX(x,y) ((x) > (y)) ? (x) : (y)

#define NOTE_FACTOR       16.3515978312874f

#define _12TH_ROOT_OF_2   1.0594630943593f
#define _1200TH_ROOT_OF_2 1.000577789506555f
#define _200TH_ROOT_OF_10 1.011579454259899f

#define centibelToRatio(x)    powf(_200TH_ROOT_OF_10, x)
#define centsToRatio(x)       powf(_1200TH_ROOT_OF_2, x)
#define centsToFreq(x)        (centsToRatio(x) * 8.176f)   // in Hz
#define centsToFreqRatio(x)   (1 / centsToFreq(x))         // in seconds
#define centsToSampleCount(x) (centsToRatio(x) * config.samplingRate)
#define noteFrequency(x)      (NOTE_FACTOR * powf(_12TH_ROOT_OF_2, x - 12))

class Utils {
public:
  inline static sampleRecord & shortToFloatNormalize(sampleRecord & dst, int16_t * src, int length)
  {
    const float32_t norm = 1.0 / 32768.0;

    assert(src != NULL);
    assert((length >= 1) && (length <= BUFFER_SAMPLE_COUNT));

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      if (length & 0x03) {
        do {
          src[length++] = 0;
        } while (length & 0x03);
      }
      assert((length >= 4) && (length <= BUFFER_SAMPLE_COUNT));

      for (int i = 0; i < length; i += 4) {
        __builtin_prefetch(&src[i]);
        int16x4_t   s16    = vld1_s16(&src[i]);
        int32x4_t   s32    = vmovl_s16(s16);
        float32x4_t f32    = vcvtq_f32_s32(s32);
        float32x4_t result = vmulq_n_f32(f32, norm);
        vst1q_f32(&dst[i], result);
      }
    #else
      for (int i = 0; i < length; i++) {
        dst[i] = src[i] * norm;
      }
    #endif
    return dst;
  }

  static inline void clip(buffp dst, frameRecord & buff)
  {
    const float minusOne = -1.0f;
    const float one      =  1.0f;

    #if USE_NEON_INTRINSICS
      // If required, pad the buffer to be a multiple of 4
      float32x4_t minusOnes = vld1q_dup_f32(&minusOne);
      float32x4_t ones = vld1q_dup_f32(&one);

      for (int i = 0; i < length; i += 2) {
        float32x4_t data = vld1q_f32(&buff[i]);
        data = vminq_f32(vmaxq_f32(data, minusOnes), ones);
        vst1q_f32(dst, data);
        dst += 4;
      }
    #else
      for (auto & element : buff) {
        *dst++  = MIN(MAX(element.left, minusOne), one);
        *dst++  = MIN(MAX(element.right, minusOne), one);
      }
    #endif
  }

  static bool fileExists(const char * name);

  inline static int16_t checkRange(int16_t value, int16_t min, int16_t max, int16_t defValue) {
    if ((value < min) || (value > max)) return defValue;
    return value;
  }
};

#endif
