#ifndef _INSTRUMENT_
#define _INSTRUMENT_

#include <string>

#include "sf2.h"

class Instrument
{
private:
  std::string name;   ///< The name of the instrument 
  bool loaded;        ///< True if the instrument content has been loaded in memory
  
public:
  Instrument(char      * instrumentName, 
             int         bagIdx, 
             int         bagCount, 
             sfBag     * bags, 
             sfGenList * generators, 
             sfModList * modulators);
  ~Instrument();
  
  /// Returns true if the instrument has been loaded in memory
  bool is_loaded() { return loaded; }
  
  /// Loads / Unloads the instrument information in memory. 
  bool load();
  bool unload();
  
  /// Returns the name of the instrument
  std::string & get_name() { return name; }
};

#endif