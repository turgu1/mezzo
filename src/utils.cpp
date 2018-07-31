#include "copyright.h"

#include <sys/stat.h>
#include <cmath>

#include "mezzo.h"
#include "utils.h"

bool Utils::fileExists(const char * name) {
  struct stat buffer;
  return (stat (name, &buffer) == 0);
}

