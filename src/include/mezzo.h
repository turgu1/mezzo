#include "copyright.h"

#ifndef _MEZZO_
#define _MEZZO_

#define SUCCESS 0

#include "globals.h"

class Mezzo   : public NewHandlerSupport<Mezzo> {

private:
  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Mezzo();
  ~Mezzo();
};

#endif
