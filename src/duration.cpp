#include "copyright.h"

#include "duration.h"

Duration::Duration()
{
  clock_gettime(CLOCK_MONOTONIC_RAW, &starttime);
}

long Duration::getElapse()
{
  struct timespec endtime;

  clock_gettime(CLOCK_MONOTONIC_RAW, &endtime);
  return (starttime.tv_sec == endtime.tv_sec) ? 
                        endtime.tv_nsec - starttime.tv_nsec : 
                        (1000000000L - starttime.tv_nsec) + endtime.tv_sec;
}
