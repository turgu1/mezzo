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
  static buffp shortToFloatNormalize(buffp dst, int16_t * src, int   len);
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
