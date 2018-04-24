#ifndef _MEZZO_
#define _MEZZO_

#define SUCCESS 0

#include <string>
#include <cassert>

#include "globals.h"

class Mezzo {
  
public:
  Mezzo(std::string & sf2_filename);
  ~Mezzo();
};

#endif
