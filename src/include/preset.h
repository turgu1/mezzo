#ifndef _PRESET_
#define _PRESET_

#include <string>

class Preset
{
private:
  std::string name;
  bool loaded;
  
public:
  Preset(std::string & preset_name);
  ~Preset();
  
  /// Returns true if the preset has been loaded in memory
  bool is_loaded() { return loaded; }
  
  /// Load the preset information in memory. That will include the
  /// loading of associated instruments.
  bool load();
  
  /// Returns the name of the preset
  std::string & get_name() { return name; }
};

#endif