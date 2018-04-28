#include "copyright.h"

#include <sys/stat.h>

#include "mezzo.h"
#include "utils.h"

buffp Utils::shortToFloatNormalize(buffp dst, int16_t * src, int len)
{
  const float norm = 1.0 / 32768;

  for (int i = 0; i < len; i++) {
    dst[i] = src[i] * norm;
  }

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

void Utils::stereoPanning(buffp dst, buffp src, int16_t pan, int len)
{
  float left  = MIN(1.0, (pan - 500.0) / -1000.0);
  float right = MIN(1.0, (pan + 500.0) /  1000.0);

  if (left < 0.001) {
    while (len--) {
      *dst++ = 0.0;
      *dst++ = *src++;
    }
  }
  else if (right < 0.001) {
    while (len--) {
      *dst++ = *src++;
      *dst++ = 0.0;
    }
  }
  else {
    while (len--) {
      *dst++ = *src * left;
      *dst++ = *src++ * right;
    }
  }
}
