#include "copyright.h"

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
#define centsToFreqRatio(x)   (1 / centsToFreq(x))        // in seconds
#define centsToSampleCount(x) (centsToRatio(x) * config.samplingRate)
#define noteFrequency(x)      (NOTE_FACTOR * powf(_12TH_ROOT_OF_2, x - 12))


class Utils {
public:
  inline static buffp shortToFloatNormalize(buffp dst, int16_t * src, int len)
  {
    #if USE_NEON_INTRINSICS
      float32_t norm = 1.0 / 32768.0;
      for (int i = 0; i < len; i += 4) {
        int16x4_t   s16    = vld1_s16(&src[i]);
        int32x4_t   s32    = vmovl_s16(s16);
        float32x4_t f32    = vcvtq_f32_s32(s32);
        float32x4_t result = vmulq_n_f32(f32, norm);
        vst1q_f32(&dst[i], result);
      }
    #else
      const float norm = 1.0 / 32768.0;

      for (int i = 0; i < len; i++) {
        dst[i] = src[i] * norm;
      }
    #endif
    return dst;
  }

  static buffp                 merge(buffp dst, buffp src,     int   len);
  static buffp        mergeAndMultBy(buffp dst, buffp src,     float ampl,     int len);
  static buffp            interleave(buffp dst, buffp srcLeft, buffp srcRight, int len);

  static bool fileExists(const char * name);

  inline static int16_t checkRange(int16_t value, int16_t min, int16_t max, int16_t defValue) {
    if ((value < min) || (value > max)) return defValue;
    return value;
  }
};

#endif
