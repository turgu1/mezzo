#include "copyright.h"

#ifndef _DURATION_
#define _DURATION_

#include <time.h>

class Duration {
 private:
  struct timespec starttime;

 public:
  Duration();
  long getElapse();
};

#endif
