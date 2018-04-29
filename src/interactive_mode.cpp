#include "copyright.h"

#include <iomanip>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "mezzo.h"
#include "interactive_mode.h"

InteractiveMode::InteractiveMode()
{
}

InteractiveMode::~InteractiveMode()
{
}

char InteractiveMode::showMenuGetSelection()
{
  using namespace std;
  char answer[6];

  cout << endl;
  cout << "Menu:" << endl << "----" << endl;

  cout << "a : Monitor active voice count   v : dump Voices state"      << endl
       << "b : Monitor midi messages        p : Show Preset Zones"      << endl
       << "e : Equalizer adjusments         i : Show Instruments Zones" << endl
       << "r : Reverb adjusments            + : Next Preset"            << endl
       << "s : Sound device selection       - : Previous Preset"        << endl
       << "m : Midi device selection"                                   << endl
       << "t : Transpose"                                               << endl
       // << "l - dump sample Library"        << endl
       // << "c - show Config read from file" << endl
       << "x : eXit"                                            << endl << endl;

  cout << "Your choice > ";
  cin >> setw(5) >> answer;

  return answer[0];
}

void InteractiveMode::menu()
{
  using namespace std;
  Preset * p;

  while (true) {
    char ch = showMenuGetSelection();

    switch (ch) {
    case 'a':
      poly->monitorCount();
      break;
    case 'b':
      midi->monitorMessages();
      break;
    case 'e':
      equalizer->interactiveAdjust();
      break;
    case 'r':
      reverb->interactiveAdjust();
      break;
    case 's':
      sound->selectDevice();
      break;
    case 'm':
      midi->selectDevice();
      break;
    case 't':
      midi->transposeAdjust();
      break;
    // case 'l':
    //   samples->showNotes();
    //   break;
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
        cout << "Please enter instrument index > ";

        string str;
        uint16_t nbr;

        cin.clear();
        cin.sync();

        cin >> str;
        if (str.empty()) break;
        istringstream iss(str);
        iss >> nbr;

        if (nbr < pi.size()) {
          Instrument * inst = soundFont->getInstrument(pi[nbr]->index);
          assert(inst != NULL);
          inst->showZones();
        }
      }
      break;
    case '+':
      p = soundFont->getCurrentPreset();
      if (p != NULL) {
        int i = p->getMidiNbr() + 1;
        soundFont->loadMidiPresetNbr(i);
      }
      break;
    case '-':
      p = soundFont->getCurrentPreset();
      if (p != NULL) {
        int i = p->getMidiNbr() - 1;
        if (i >= 0) {
          soundFont->loadMidiPresetNbr(i);
        }
        else {
          cout << "Already on first preset" << endl;
        }
      }
      break;
    case 'v':
      poly->showState();
      break;
    // case 'c':
    //   config->showState();
    //   break;
    case 'x':
      return;
    default:
      cout << "Bad entry!" << endl;
      break;
    }
  }
}
