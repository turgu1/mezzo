#include "copyright.h"

#include <iomanip>

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

  cout << "a - Monitor active voice count" << endl
       << "b - Monitor midi messages"      << endl
       << "e - Equalizer adjusments"       << endl
       << "r - Reverb adjusments"          << endl
       << "s - Sound device selection"     << endl
       << "m - Midi device selection"      << endl
       << "t - Transpose"                  << endl
       << "l - dump sample Library"        << endl
       << "v - dump Voices state"          << endl
       << "c - show Config read from file" << endl
       << "x - eXit"                       << endl << endl;

  cout << "Your choice > ";
  cin >> setw(5) >> answer;

  return answer[0];
}

void InteractiveMode::menu()
{

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
      sound->deviceSelect();
      break;
    case 'm':
      midi->deviceSelect();
      break;
    case 't':
      midi->transposeAdjust();
      break;
    case 'l':
      samples->showNotes();
      break;
    case 'v':
      poly->showState();
      break;
    case 'c':
      config->showState();
      break;
    case 'x':
      return;
    default:
      std::cout << "Bad entry!" << std::endl;
      break;
    }
  }
}
