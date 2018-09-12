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

#include "lcd_keypad.h"

int  vol   =    70;
int  bpm   =    70;
int  bpms  =     4;
bool run   = false;
bool metEn = false;

LcdKeypad::choiceEntry yesNo[] = {
  { "Yes",  1 },
  { "No",   0 },
  { "",    -1 }
};

LcdKeypad::choiceEntry beats[] = {
  { "2",   2},
  { "3",   3},
  { "4",   4},
  { "6",   6},
  { "9",   9},
  { "12", 12},
  { "18", 18},
  { "24", 24},
  { "",   -1}
};

void setMetActive(int v) {
  if (config.metEnabled) {
    if (v) {
      metronome->start();
    }
    else {
      metronome->stop();      
    }
  }
  else {
    metronome->stop();
  }
}

int  getMetEnabled()      { return config.metEnabled;                }
void setMetEnabled(int v) {        config.metEnabled = v; if (!v) metronome->stop(); }
int   getMetActive()      { return metronome->isActive();            }
int    getMetBPMin()      { return metronome->getBeatsPerMinute();   }
void   setMetBPMin(int v) {        metronome->setBeatsPerMinute(v);  }
int    getMetBPMea()      { return metronome->getBeatsPerMeasure();  }
void   setMetBPMea(int v) {        metronome->setBeatsPerMeasure(v); }

#define M menus
#define K LcdKeypad
#define c config
#define m metronome

LcdKeypad::menuEntry M[] = {
  //  label           setFunc        getFunc        next    prev    sub     param type       low,  high,  choices
  {  "Mezzo V1.0",    NULL,          NULL,          &M[ 1], &M[ 1], &M[ 6], NULL, K::NONE,   0,    0,     NULL   }, // 0
  {  "Metronome",     NULL,          NULL,          &M[ 0], &M[ 0], &M[ 2], NULL, K::NONE,   0,    0,     NULL   }, // 1

  {  "Enable",        setMetEnabled, getMetEnabled, &M[ 3], &M[ 5], NULL,   NULL, K::BOOL,   0,    0,     yesNo  }, // 2
  {  "Running",       setMetActive,  getMetActive,  &M[ 4], &M[ 2], NULL,   NULL, K::BOOL,   0,    0,     yesNo  }, // 3
  {  "Beats/Minute",  setMetBPMin,   getMetBPMin,   &M[ 5], &M[ 3], NULL,   NULL, K::INT,   10,  250,     NULL   }, // 4
  {  "Beats/Measure", setMetBPMea,   getMetBPMea,   &M[ 2], &M[ 4], NULL,   NULL, K::INT,    0,    0,     beats  }, // 5

  {  "Volume",        NULL,          NULL,          &M[ 6], &M[ 6], NULL,   &vol, K::INT,    0,  100,     NULL   }  // 6
};

#undef M
#undef K
#undef c
#undef m

LcdKeypad::LcdKeypad(std::string & devName) 
{
  deviceName  = devName;
  keypadFd    = -1;
  state       = WAIT_RESET;
  initMenu();
}

void LcdKeypad::initMenu()
{
  currentMain  = &menus[0];
  currentSub   =  currentMain->subMenu;

  mainString    = "";
  editString    = "";
  subString     = "";
  prmString     = "";
  emptyString   = "";
  prmLocation   = -1;
  voiceLocation = -1;

  getCurrentValue();
}

void LcdKeypad::getCurrentValue()
{
  if (currentSub != NULL) {
    if (currentSub->param != NULL) {
      switch (currentSub->paramType) {
        case INT:
          currentValue = * (int *) currentSub->param;
          break;
        case INT16:
          currentValue = * (int16_t *) currentSub->param;
          break;
        case INT8:
          currentValue = * (int8_t *) currentSub->param;
          break;
        case BOOL:
          currentValue = * (bool *) currentSub->param;
          break;
        case NONE:
          currentValue = 0;
          break;
      }
    }
    else if (currentSub->getFunc != NULL) {
      currentValue = (*currentSub->getFunc)();
    }
  }
  oldValue = currentValue;
}

void LcdKeypad::setCurrentValue()
{
  if (currentSub != NULL) {
    if (currentSub->param != NULL) {
      switch (currentSub->paramType) {
        case INT:
          * (int *) currentSub->param = currentValue;
          break;
        case INT16:
          * (int16_t *) currentSub->param = currentValue;
          break;
        case INT8:
          * (int8_t *) currentSub->param = currentValue;
          break;
        case BOOL:
          * (bool *) currentSub->param = currentValue;
          break;
        case NONE:
          break;
      }
    }
    else if (currentSub->setFunc != NULL) {
      (*currentSub->setFunc)(currentValue);
    }
    oldValue = currentValue;
  }
}

int LcdKeypad::findChoice()
{
  int i = 0;
  while (currentSub->choices[i].value != -1) {
    if (currentValue == currentSub->choices[i].value) {
      return i;
    }
    i++;
  }
  return -1;
}

void LcdKeypad::nextValue(bool fast)
{
  if (currentSub->choices == NULL) {
    if (fast) {
      currentValue += 10;
    }
    else {
      currentValue++;
    }
    if (currentValue > currentSub->high) currentValue = currentSub->high;
  }
  else {
    int i = findChoice() + 1;
    if (currentSub->choices[i].value != -1) {
      currentValue = currentSub->choices[i].value;
    }
    else {
      currentValue = currentSub->choices[0].value;
    }
  }
}

void LcdKeypad::previousValue(bool fast)
{
  if (currentSub->choices == NULL) {
    if (fast) {
      currentValue -= 10;
    }
    else {
      currentValue--;
    }
    if (currentValue < currentSub->low) currentValue = currentSub->low;
  }
  else {
    int i = findChoice() - 1;
    if (i == -1) {
      i = 1;
      while (currentSub->choices[i].value != -1) i++;
      i--;
    }
    currentValue = currentSub->choices[i].value;
  }
}

std::string & LcdKeypad::getChoiceString()
{
  int i = findChoice();
  if (i >= 0) {
    return currentSub->choices[i].label;
  }
  else {
    return emptyString;
  }
}

std::string & LcdKeypad::paramString()
{
  static std::string res;
  if (currentSub->choices == NULL) {
    res = std::to_string(currentValue);
  }
  else {
    res = getChoiceString();
  }
  if ((oldValue == currentValue) && (state == DATA_ENTRY)) res = '*' + res;
  return res;
}

#define show(d) write(keypadFd, d.c_str(), d.size())

void LcdKeypad::showMenu()
{
  std::string data;
   
  data = "0,0C:" + currentMain->label + "\n";
  if (data != mainString) {
    mainString = data;
    show(data);

    if (state == DATA_ENTRY) {
      editString = "0,12:Edit\n";
      show(editString);
    }
    else {
      editString = "0,12C:\n";        
    }
  }

  if (state == DATA_ENTRY) {
    data = "0,12:Edit\n";
  }
  else {
    data = "0,12C:\n";
  }
  if (data != editString) {
    editString = data;
    show(editString);
  }

  if (currentSub != NULL) {
    data = "1,0C:" + currentSub->label + "\n";
    if (data != subString) {
      subString = data;
      show(data);

      if ((currentSub->param != NULL) || (currentSub->getFunc != NULL)) {
        std::string p = paramString();
        prmLocation = 16 - p.size();
        data = "1," + std::to_string(prmLocation) + ":" + p + "\n";
        prmString = data;
        show(data);
      }
      else {
        prmLocation = -1;
      }
    }
    else {
      if ((currentSub->param != NULL) || (currentSub->getFunc != NULL)) {
        std::string p = paramString();
        data = "1," + std::to_string(16 - p.size()) + ":" + p + "\n";
        if (data != prmString) {
          prmString = data;
          if (prmLocation != -1) {
            std::string data2 = "1," + std::to_string(prmLocation) + "C:\n";
            show(data2);
          }
          prmLocation = 16 - p.size();
          show(data);
        }
      }
      else {
        prmLocation = -1;
      }         
    }
  }
  else {
    subString = "1,0C:\n";
    show(subString);
  }
}

void LcdKeypad::initMonitor()
{
  mainString    = "";
  subString     = "";
  prmString     = "";
  vceString     = "";
  prmLocation   = -1;
  voiceLocation = -1;
}

void LcdKeypad::showMonitor()
{
  std::string data = "0,0C:" + soundFont->getCurrentPreset()->getName().substr(0, 16) + "\n";
  if (mainString != data) {
    mainString = data;
    show(data);
  }

  data = "1,0C:Vol     Vces\n";
  if (subString != data) {
    subString = data;
    show(data);
  }

  std::string volString = std::to_string(config.volume);
  data = "1," + std::to_string(7 - volString.size()) + ":" + volString + "\n";
  if (prmString != data) {
    if (prmLocation != -1) {
      std::string data2 = "1,4:   \n";
      show(data2);
    }
    prmLocation = 7 - volString.size();
    prmString = data;
    show(data);
  }

  std::string voiceString = std::to_string(poly->getVoiceCount());
  data = "1," + std::to_string(16 - voiceString.size()) + ":" + voiceString + "\n";
  if (vceString != data) {
    if (voiceLocation != -1) {
      std::string data2 = "1,13:   \n";
      show(data2);
    }
    voiceLocation = 16 - voiceString.size();
    vceString = data;
    show(data);
  }
}

char LcdKeypad::getNextKey()
{
  do {
    if (keypadFd == -1) {
      bool first = true;
      do {
        if (!first) sleep(1);
        first = false;
        keypadFd = open(deviceName.c_str(), O_RDWR);
      } while (keypadFd == -1);
      state = WAIT_RESET;
    }

    char line[6];
    struct timeval timeout;
    fd_set  set, set2;

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    FD_ZERO(&set);
    FD_SET(keypadFd, &set);

    while (true) {
      memcpy(&set2, &set, sizeof(set));
      int rc = select(keypadFd + 1, &set2, NULL, NULL, &timeout);
      if (rc == -1) {
        perror("select failed!");
        return 0;
      }
      else if (rc == 1) {
        int cnt = read(keypadFd, line, 5);
        if (cnt == -1) break;
        if (cnt !=  2) continue;
        return line[0];
      }
      else {
        return 0;
      }
    }

    close(keypadFd);
    keypadFd = -1;

  } while (true);

  return 0;
}

void LcdKeypad::process() 
{  
  char key = getNextKey();

  //std::cout << "Key: " << (key == 0 ? '#' : key) << std::endl;

  switch (state) {
  case WAIT_RESET:
    if (key == RESET) {
      state = MONITOR;
      initMonitor();
      showMonitor();
    }
    break;

  case MONITOR:
    switch (key) {
      case SELECT:
        state = NAVIGATE;
        initMenu();
        showMenu();
        return;
      case LEFT:
        if (config.volume > 0) {
          config.volume--;
          config.masterVolume = config.volume / 100.0f;
        }
        break;
      case RIGHT:
        if (config.volume < 100) {
          config.volume++;
          config.masterVolume = config.volume / 100.0f;
        }
        break;
      case DOWN:
        soundFont->loadNextPreset();
        break;  
      case UP:
        soundFont->loadPreviousPreset(); 
        break;
      case NOTHING:
        break;
    }
    showMonitor();
    break;

  case NAVIGATE:
    switch (key) {
      case RESET:
        state = MONITOR;
        initMonitor();
        showMonitor();
        return;
      case RIGHT:
        if (currentMain->next != NULL) {
          currentMain = currentMain->next;
          currentSub  = currentMain->subMenu;
          getCurrentValue();
        }
        break;
      case LEFT:
        if (currentMain->prev != NULL) {
          currentMain = currentMain->prev;
          currentSub  = currentMain->subMenu;
          getCurrentValue();
        }
        break;
      case UP:
        if ((currentSub != NULL) && (currentSub->prev != NULL)) {
          currentSub = currentSub->prev;
          getCurrentValue();
        }
        break;
      case DOWN:
        if ((currentSub != NULL) && (currentSub->next != NULL)) {
          currentSub = currentSub->next;
          getCurrentValue();
        }
        break;
      case SELECT:
        // if ((currentSub != NULL) && (currentSub->param != NULL)) {
        //   setCurrentValue();
        // }
        state = DATA_ENTRY;
        break;
      case NOTHING:
        break;
    }
    showMenu();
    break;

  case DATA_ENTRY:
    switch (key) {
      case RESET:
        initMenu();
        state = NAVIGATE;
        break;
      case UP:
        previousValue();
        break;
      case DOWN:
        nextValue();
        break;
      case LEFT:
        previousValue(true);
        break;
      case RIGHT:
        nextValue(true);
        break;
      case SELECT:
        state = NAVIGATE;
        setCurrentValue();
        break;
      case NOTHING:
        break;
    }
    showMenu();
    break;
  }
}