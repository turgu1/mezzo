#include "copyright.h"

#ifndef _UTILS_
#define _UTILS_

buffp shortToFloatNormalize(buffp dst, int16_t * src, int   len);
buffp                 merge(buffp dst, buffp src,     int   len);
buffp        mergeAndMultBy(buffp dst, buffp src,     float ampl,     int len);
buffp            interleave(buffp dst, buffp srcLeft, buffp srcRight, int len);
void          stereoPanning(buffp dst, buffp src, int16_t pan, int len);

bool fileExists(const char * name);

#endif