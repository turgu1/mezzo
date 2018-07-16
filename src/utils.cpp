#include "copyright.h"

#include <sys/stat.h>
#include <cmath>

#include "mezzo.h"
#include "utils.h"

buffp Utils::shortToFloatNormalize(buffp dst, int16_t * src, int len)
{
  #if USE_NEON_INTRINSICS
    float32_t norm = 1.0 / 32768.0;
    for (int i = 0; i < len; i += 4) {
      int16x4_t   i16    = vld1_s16(&src[i]);
      int32x4_t   i32    = vmovl_s16(i16);
      float32x4_t f32    = vcvt_f32_s32(i32);
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

buffp Utils::merge(buffp dst, buffp src, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i] += src[i];
  }

  return dst;
}

buffp Utils::mergeAndMultBy(buffp dst, buffp src, float ampl, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i] = (dst[i] + src[i]) * ampl;
  }

  return dst;
}

buffp Utils::interleave(buffp dst, buffp srcLeft, buffp srcRight, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i * 2]     = srcLeft[i];
    dst[i * 2 + 1] = srcRight[i];
  }

  return dst;
}

bool Utils::fileExists(const char * name) {
  struct stat buffer;
  return (stat (name, &buffer) == 0);
}

