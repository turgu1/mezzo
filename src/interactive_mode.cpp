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

#include <iomanip>
#include <iostream>
#include <sstream>

#include "mezzo.h"

InteractiveMode::InteractiveMode()
{
}

InteractiveMode::~InteractiveMode()
{
}

char InteractiveMode::showMenuGetSelection(bool showMenu)
{
  using namespace std;

  if (showMenu) {
      cout          << endl
         << "Menu:" << endl 
         << "----"  << endl
         << "P : Select Preset            A : Monitor active voice count"      << endl
         << "+ : Next Preset              B : Monitor midi messages"           << endl
         << "- : Previous Preset          E : Equalizer adjusments "           << endl
         << "T : Transpose                R : Reverb adjusments"               << endl
         << "S : Sound device selection   V : Show Voices state while playing" << endl
         << "M : Midi device selection    p : Show Preset Zones"               << endl
         << "f : toggle low-pass filter   i : Show Instruments Zones"          << endl
         << "e : toggle envelope" << endl
         << "v : toggle vibrato"  << endl
         // << "l - dump sample Library"        << endl
         // << "c - show Config read from file" << endl
         << "x : eXit                     ? : Show this menu"                  << endl << endl;

    cout << "Your choice > " << flush;
  }

  string answer;

  bool timeout = Utils::readStdIn(answer);

  return timeout ? -1 : answer[0];
}

void InteractiveMode::menu()
{
  using namespace std;
  Preset * p;
  int16_t nbr;

  bool showIt = true;

  while (true) {

    signed char ch = showMenuGetSelection(showIt);
    if (!keepRunning) break;

    switch (ch) {
    case 'x': return;
    case 'A': poly->monitorCount();            break;
    case 'B': midi->monitorMessages();         break;
    case 'E': equalizer->interactiveAdjust();  break;
    case 'R': reverb->interactiveAdjust();     break;
    case 'S': {
      int devNbr = sound->selectDevice(-1);
      sound->openPort(devNbr); }
      break;
    case 'M': {
      int devNbr = midi->selectDevice(-1);
      midi->openPort(devNbr); }
      break;
    case 'T': midi->transposeAdjust();         break;
    // case 'l': samples->showNotes();         break;
    case 'V': 
      poly->showVoicePlayingState();   
      cout << "Voice State while playing is now "
           << (Voice::isPlayingStateActive() ? "Active" : "Inactive")
           << endl;
      break;
    // case 'c': config->showState();          break;
    
    case '+': 
      soundFont->loadNextPreset();
      cout << soundFont->getCurrentPreset()->getName() << " Selected." << endl;
      break;  
          
    case '-': 
      soundFont->loadPreviousPreset(); 
      cout << soundFont->getCurrentPreset()->getName() << " Selected." << endl;
      break;
    
    case 'P': 
      {
        std::vector<uint16_t> theList = soundFont->showMidiPresetList();
        while (true) {
          cout << endl << "Please enter preset index > " << flush;
          do {
            Utils::getNumber(nbr);
          } while (nbr < 0);

          if ((nbr >= 1) && (((uint16_t) nbr) <= theList.size())) {
            soundFont->loadPreset(theList[nbr - 1]);
            cout << "=====> " << soundFont->getCurrentPreset()->getName() << " Selected. <=====" << endl;
            break;
          }
          else {
            cout << "Index out of range! Please Try Again." << endl;
          }
        }
      }
      break;      
      
    case 'p':
      p = soundFont->getCurrentPreset();
      if (p != NULL) p->showZones();
      break;
      
    case 'i':
      p = soundFont->getCurrentPreset();
      if (p != NULL) {
        vector<presetInstrument *> & pi = p->getInstrumentsList();
        for (unsigned i = 0; i < pi.size(); i++) {
          cout << i << " : " << pi[i]->name << endl;
        }
        cout << "Please enter instrument index > " << flush;

        do {
          Utils::getNumber(nbr);
        } while (nbr < 0);

        if ((nbr >= 0) && (((uint16_t) nbr) < pi.size())) {
          Instrument * inst = soundFont->getInstrument(pi[((uint16_t) nbr)]->index);
          assert(inst != NULL);
          inst->showZones();
        }
      }
      break;

    case 'f':
      Synthesizer::toggleFilter();
      cout << "Low-Pass Filters are now "
           << (Synthesizer::areAllFilterActive() ? "Active" : "Inactive") 
           << endl;
      break;
      
    case 'e':
      Synthesizer::toggleEnvelope();
      cout << "Envelopes are now "
           << (Synthesizer::areAllEnvelopeActive() ? "Active" : "Inactive") 
           << endl;
      break;

    case 'v':
      Synthesizer::toggleVibrato();
      cout << "Vibratos are now "
           << (Synthesizer::areAllVibratoActive() ? "Active" : "Inactive") 
           << endl;
      break;

    case '?': break;

    case -1: // Timeout happened
      showIt = false;
      continue;

    case 0:  // Empty string
      break;

    default:
      cout << "Bad entry! (" << +ch << ")"
           << endl;
      break;
    }

    showIt = true;
  }
}
