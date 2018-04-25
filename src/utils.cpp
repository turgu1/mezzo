#include "copyright.h"

#include <sys/stat.h>

#include "mezzo.h"
#include "utils.h"

buffp shortToFloatNormalize(buffp dst, int16_t * src, int len)
{
  const float norm = 1.0 / 32768;
  
  for (int i = 0; i < len; i++) {
    dst[i] = src[i] * norm;
  }
  
  return dst;
}

buffp merge(buffp dst, buffp src, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i] += src[i];
  }

  return dst;
}

buffp mergeAndMultBy(buffp dst, buffp src, float ampl, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i] = (dst[i] + src[i]) * ampl;
  }  

  return dst;
}

buffp interleave(buffp dst, buffp srcLeft, buffp srcRight, int len)
{
  for (int i = 0; i < len; i++) {
    dst[i * 2]     = srcLeft[i];
    dst[i * 2 + 1] = srcRight[i];
  }  

  return dst;
}

bool fileExists(const char * name) {
  struct stat buffer;   
  return (stat (name, &buffer) == 0); 
}
