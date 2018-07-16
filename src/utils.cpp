#include "copyright.h"

#include <sys/stat.h>
#include <cmath>

#include "mezzo.h"
#include "utils.h"

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

