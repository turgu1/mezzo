#include "copyright.h"

#ifndef INTERACTIVE_MODE_H
#define INTERACTIVE_MODE_H

/// This class implement the interactive behavior. It allows a user to
/// access some predefined application features, modifying some of the
/// parameters or accessing internal states to comprehend Mezzo working
/// conditions. It is also helpfull to debug the application.

class InteractiveMode {

 private:
  char showMenuGetSelection();
  int16_t getNumber();
  
 public:
   InteractiveMode();
  ~InteractiveMode();
  void menu();
};

#endif
