#include "copyright.h"

#ifndef _UTILS_
#define _UTILS_

#define _1200TH_ROOT_OF_2 1.000577789506555
#define _200TH_ROOT_OF_10 1.011579454259899

#define centibelToRatio(x)   powf(_200TH_ROOT_OF_10, x)
#define centsToRatio(x)      powf(_1200TH_ROOT_OF_2, x)
#define centsToFreq(x)       (centsToRatio(x) * 8.176)   // in Hz
#define centsToFreqRatio(x)  (1 / centsToFreq(x))        // in seconds
#define centsToDuration(x)   (centsToFreqRatio(x) * config.samplingRate)  // in samples count

class Utils {
public:
  static buffp shortToFloatNormalize(buffp dst, int16_t * src, int   len);
  static buffp                 merge(buffp dst, buffp src,     int   len);
  static buffp        mergeAndMultBy(buffp dst, buffp src,     float ampl,     int len);
  static buffp            interleave(buffp dst, buffp srcLeft, buffp srcRight, int len);

  static bool fileExists(const char * name);
};

#endif
