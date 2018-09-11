// Notice
// ------
//
// This file is part of the Mezzo SoundFont2 Sampling Based Synthesizer:
//
//     https://github.com/turgu1/mezzo
//
// Simplified BSD License
// ----------------------
//
// Copyright (c) 2018, Guy Turcotte
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//

#ifndef _LCD_KEYPAD_
#define _LCD_KEYPAD_

#include "mezzo.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#define UP      'U'
#define DOWN    'D'
#define LEFT    'L'
#define RIGHT   'R'
#define SELECT  'S'
#define RESET   'I'
#define NOTHING  0

class LcdKeypad {
public:

  enum ParamType { NONE = 0, INT, INT16, INT8, BOOL };

  struct choiceEntry {
    std::string label;
    int         value;
  };

  struct menuEntry {
    std::string    label;        ///< Label to show on LCD
    void        (* setFunc)(int v);   ///< Function to call when value modified
    int         (* getFunc)();   ///< Function to call to get value as an int
    menuEntry    * next;         ///< Next entry to show on RIGHT on main or DOWN on sub
    menuEntry    * prev;         ///< Next entry to show on LEFT  on main or UP on sub
    menuEntry    * subMenu;      ///< First sub-menu entry for main menu entries
    void         * param;        ///< Parameter location
    ParamType      paramType;    ///< Parameter type (one of ParamType)
    int            low;          ///< Lowest value permitted for that parameter
    int            high;         ///< Highest value permitted for that parameter
    choiceEntry  * choices;      ///< Choices to select from for pre-defined parameter values
  };

  LcdKeypad(std::string & devName);
 ~LcdKeypad() {}

  void process();

private:

  enum State { WAIT_RESET, MONITOR, NAVIGATE, DATA_ENTRY };

  std::string deviceName;
  int keypadFd;
  State state;

  menuEntry * currentMain;
  menuEntry * currentSub;

  int currentValue;
  int oldValue;

  std::string    mainString;
  std::string    editString;
  std::string     subString;
  std::string     prmString;
  std::string   emptyString;
  std::string     vceString;
  int           prmLocation;
  int         voiceLocation;

  void initMenu();
  void getCurrentValue();
  void setCurrentValue();
  int  findChoice();
  void nextValue(bool fast = false);
  void previousValue(bool fast = false);
  std::string & getChoiceString();
  std::string & paramString();
  void showMenu();
  void initMonitor();
  void showMonitor();
  char getNextKey();

};

#endif