#include "copyright.h"

#ifndef _MEZZO_
#define _MEZZO_

#include "globals.h"
#include "utils.h"

#include "sf2.h"

#include "sample.h"
#include "synthesizer.h"
#include "preset.h"
#include "instrument.h"
#include "soundfont2.h"
#include "midi.h"
#include "reverb.h"
#include "sound.h"
#include "voice.h"
#include "poly.h"
#include "equalizer.h"
#include "reverb.h"

class Mezzo   : public NewHandlerSupport<Mezzo> {

private:
  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Mezzo();
  ~Mezzo();
};

#endif
