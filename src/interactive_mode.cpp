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

  cout <<            endl
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
  int16_t nbr;

  while (true) {
    char ch = showMenuGetSelection();

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
          cout << endl << "Please enter preset index > ";
          nbr = getNumber();
          if (nbr < 0) break;

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
        cout << "Please enter instrument index > ";

        nbr = getNumber();
        if (nbr < 0) break;

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

    default:
      cout << "Bad entry!" 
           << endl;
      break;
    }
  }
}
