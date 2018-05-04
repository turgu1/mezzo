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

  cout << "a : Monitor active voice count   P : Select Preset"          << endl
       << "b : Monitor midi messages        p : Show Preset Zones"      << endl
       << "e : Equalizer adjusments         i : Show Instruments Zones" << endl
       << "r : Reverb adjusments            + : Next Preset"            << endl
       << "s : Sound device selection       - : Previous Preset"        << endl
       << "m : Midi device selection        V : dump Voices state"      << endl
       << "t : Transpose                    f : toggle low-pass filter" << endl
       // << "l - dump sample Library"        << endl
       // << "c - show Config read from file" << endl
       << "x : eXit"                                         << endl << endl;

  cout << "Your choice > ";
  cin >> setw(5) >> answer;

  return answer[0];
}

int16_t InteractiveMode::getNumber()
{
  using namespace std;

  string str;
  uint16_t nbr;

  cin.clear();
  cin.sync();

  cin >> str;
  if (str.empty()) return -1;
  istringstream iss(str);
  iss >> nbr;
  
  return nbr;
}

void InteractiveMode::menu()
{
  using namespace std;
  Preset * p;
  int16_t  nbr;

  while (true) {
    char ch = showMenuGetSelection();

    switch (ch) {
    case 'x': return;
    case 'a': poly->monitorCount();            break;
    case 'b': midi->monitorMessages();         break;
    case 'e': equalizer->interactiveAdjust();  break;
    case 'r': reverb->interactiveAdjust();     break;
    case 's': sound->selectDevice();           break;
    case 'm': midi->selectDevice();            break;
    case 't': midi->transposeAdjust();         break;
    // case 'l': samples->showNotes();         break;
    case 'v': poly->showState();               break;
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
      soundFont->showMidiPresetList();
      cout << endl << "Please enter preset index > ";
      nbr = getNumber();
      if (nbr >= 0) soundFont->loadPreset(nbr);
      cout << soundFont->getCurrentPreset()->getName() << " Selected." << endl;
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
        cout << "Please enter instrument index > ";

        nbr = getNumber();

        if ((nbr >= 0) && ((unsigned)nbr < pi.size())) {
          Instrument * inst = soundFont->getInstrument(pi[nbr]->index);
          assert(inst != NULL);
          inst->showZones();
        }
      }
      break;

    case 'f':
      Synthesizer::toggleFilter();
      cout << "Low-Pass Filter is now "
           << (Synthesizer::isFilterEnabled() ? "Enable" : "Disable") << endl;
      break;
      
    default:
      cout << "Bad entry!" << endl;
      break;
    }
  }
}
