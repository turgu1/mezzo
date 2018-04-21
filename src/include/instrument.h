#ifndef _INSTRUMENT_
#define _INSTRUMENT_

#include <string>

class Instrument
{
private:
  std::string name;
  bool loaded;
  
public:
  Instrument(std::string & instrument_name);
  ~Instrument();
  
  /// Returns true if the instrument has been loaded in memory
  bool is_loaded() { return loaded; }
  
  /// Load the instrument information in memory. 
  bool load();
  
  /// Returns the name of the instrument
  std::string & get_name() { return name; }
};

#endif