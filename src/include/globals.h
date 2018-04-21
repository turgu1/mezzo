#ifndef _GLOBALS_
#define _GLOBALS_

#ifdef GLOBALS
  #define PUBLIC
#else
  #define PUBLIC extern
#endif

#include "log.h"

class Mezzo;
class SoundFont2;

PUBLIC volatile bool keepRunning;
PUBLIC bool          interactive;
PUBLIC bool          silent;

PUBLIC Mezzo      * mezzo;
PUBLIC SoundFont2 * soundFont;

PUBLIC Log logger;

#endif