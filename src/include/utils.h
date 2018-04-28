#include "copyright.h"

#ifndef _UTILS_
#define _UTILS_

class Utils {
public:
  static buffp shortToFloatNormalize(buffp dst, int16_t * src, int   len);
  static buffp                 merge(buffp dst, buffp src,     int   len);
  static buffp        mergeAndMultBy(buffp dst, buffp src,     float ampl,     int len);
  static buffp            interleave(buffp dst, buffp srcLeft, buffp srcRight, int len);
  static void          stereoPanning(buffp dst, buffp src, int16_t pan, int len);

  static bool fileExists(const char * name);
};

#endif
