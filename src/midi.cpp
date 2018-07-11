#include "copyright.h"

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>

#include "mezzo.h"

//PRIVATE const int MIDI_EVENT_COUNT  = 50;

//PRIVATE const int MIDI_STATUS_MASK  = 0x80;

PRIVATE const int MIDI_CHANNEL_MASK = 0x0F;
PRIVATE const int MIDI_COMMAND_MASK = 0xF0;

PRIVATE const int MIDI_NOTE_OFF     = 0x80;
PRIVATE const int MIDI_NOTE_ON      = 0x90;
PRIVATE const int MIDI_POLY_AT      = 0xA0;
PRIVATE const int MIDI_CONTROL      = 0xB0;
PRIVATE const int MIDI_PROGRAM      = 0xC0;
PRIVATE const int MIDI_CHANNEL_AT   = 0xD0;
PRIVATE const int MIDI_PITCHBEND    = 0xE0;

PRIVATE const int MIDI_SYSEX        = 0xF0;
PRIVATE const int MIDI_MTC          = 0xF1;
PRIVATE const int MIDI_SONGPOS      = 0xF2;
PRIVATE const int MIDI_SONGSEL      = 0xF3;
PRIVATE const int MIDI_TUNE         = 0xF6;
PRIVATE const int MIDI_EOX          = 0xF7;
PRIVATE const int MIDI_CLOCK        = 0xF8;
PRIVATE const int MIDI_F9           = 0xF9;
PRIVATE const int MIDI_START        = 0xFA;
PRIVATE const int MIDI_CONTINUE     = 0xFB;
PRIVATE const int MIDI_STOP         = 0xFC;
PRIVATE const int MIDI_FD           = 0xFD;
PRIVATE const int MIDI_ACTIVE       = 0xFE;
PRIVATE const int MIDI_RESET        = 0xFF;

static struct midiNameStruct {
  const int value;
  const char * name;
} midiNames[22] = {
  { MIDI_NOTE_OFF,   "Note Off"           },
  { MIDI_NOTE_ON,    "Note On"            },
  { MIDI_POLY_AT,    "Poly Aftertouch"    },
  { MIDI_CONTROL,    "Control Change"     },
  { MIDI_PROGRAM,    "Program Change"     },
  { MIDI_CHANNEL_AT, "Channel Aftertouch" },
  { MIDI_PITCHBEND,  "Pitch Bend"         },
  { MIDI_SYSEX,      "System Exclusive"   },
  { MIDI_MTC,        "Midi Time Code"     },
  { MIDI_SONGPOS,    "Song Position"      },
  { MIDI_SONGSEL,    "Song Selection"     },
  { MIDI_TUNE,       "Tune"               },
  { MIDI_EOX,        "EOX"                },
  { MIDI_CLOCK,      "Clock"              },
  { MIDI_F9,         "Undefined"          },
  { MIDI_START,      "Start"              },
  { MIDI_CONTINUE,   "Continue"           },
  { MIDI_STOP,       "Stop"               },
  { MIDI_FD,         "FD"                 },
  { MIDI_ACTIVE,     "Active"             },
  { MIDI_RESET,      "Reset"              },
  { 0,               "Undefined"          }
};

//---- miniName() ----

static const char * midiName(int value)
{
  struct midiNameStruct *md = midiNames;

  while (true) {
    if (md->value == value) break;
    if (md->value == 0) break;
    md++;
  }
  return md->name;
}

//---- midiCallBack ----

void midiCallBack (double timeStamp,
                   std::vector<unsigned char> * message,
                   void * userData)
{
  //DEBUG("midiCallBack() ...\n");

  (void) timeStamp;
  (void) userData;

  static uint8_t bankNbr = 0;

  int count = message->size();

  if (count <= 0) return;

  int channel = message->at(0) & MIDI_CHANNEL_MASK;

  uint8_t command = message->at(0) & ((uint8_t) MIDI_COMMAND_MASK);
  uint8_t data1 = (count > 1) ? message->at(1) : 0;
  uint8_t data2 = (count > 2) ? message->at(2) : 0;

  if (command == MIDI_SYSEX) {
      command = message->at(0);
      channel = -1;
  }

  if (((command & 0xF0) == MIDI_SYSEX) ||
      ((1 << channel) & midi->channelMask)) {

    switch (command) {
    case MIDI_NOTE_ON:
      midi->setNoteOn(data1 + config.midiTranspose, data2);
      break;
    case MIDI_NOTE_OFF:
      midi->setNoteOff(data1 + config.midiTranspose, data2);
      break;
    case MIDI_CONTROL:
      switch (data1) {
      case 0x20:
        bankNbr = data2;
        break;
      case 0x40:
        if (midi->sustainIsOn() && (data2 < config.midiSustainTreshold)) {
          midi->setSustainOff();
        }
        else if (!midi->sustainIsOn() && (data2 >= config.midiSustainTreshold)) {
          midi->setSustainOn();
        }
        break;
      case 0x47:
        reverb->setRoomSize(0.7f + 0.29f * (data2 / 127.0f));
        break;
      case 0x4A:
        config.masterVolume = data2 / 127.0f;
        break;
      default:
          //logger.WARNING("Midi: Ignored Control: %02xh %d.\n",
          //               data1, data2);
        break;
      }
      break;
    case MIDI_PROGRAM:
      if (!sound->holding()) {
        sound->wait();
        poly->inactivateAllVoices();
        soundFont->loadMidiPreset(bankNbr, data1);
        sound->conti();
      }
      break;
    default:
        //logger.WARNING("Midi: Ignored Event: %02xh %d %d.\n",
        //             command, data1, data2);
      break;
    }
  }

  using namespace std;

  if (midi->monitoring) {
    cout << "[ (" << hex << (int)command << "h : " << midiName(command) << ")";
    if (command == MIDI_SYSEX) {
      for (int i = 1; i < count; i++) {
        cout << ", " << dec << message->at(i);
      }
      cout << " ]";
    }
    else {
      cout << ", " << dec << (int)data1 << ", " << (int)data2 << " ]";
    }
    cout << endl << flush;
  }
}

//---- showDevices() ----

void Midi::showDevices(int devCount)
{
  using namespace std;

  cout << endl << endl;
  cout << "MIDI Input Device list:" << endl;
  cout << "----------------------"  << endl;

  for (int i = 0; i < devCount; i++) {
    cout << "Device " << i << ": " <<  midiPort->getPortName(i) << endl;
  }
  cout << "[End of list]" << endl << endl;
}

//---- selectDevice() ----

int Midi::selectDevice(int defaultNbr)
{
  using namespace std;

  int devCount;
  int devNbr = -1;

  devCount = midiPort->getPortCount();

  showDevices(devCount);

  while (true) {

    char userData[6];
    int userNbr;

    cout << "Please enter MIDI device number to use [" << defaultNbr << "]> ";

    cin.getline(userData, 5);

    if (strlen(userData) == 0) {
      devNbr = defaultNbr;
      break;
    }

    userNbr = atoi(userData);

    if ((userNbr < 0) || (userNbr >= devCount)) {
      cout << "!! Invalid device number[" << userNbr << "]. Please try again !!" << endl;
    }
    else {
      devNbr = userNbr;
      break;
    }
  }

  cout << "MIDI Device Selected: " << devNbr << endl;

  return devNbr;
}

//---- findDeviceNbr() ----

int Midi::findDeviceNbr()
{
  int devCount;
  int devNbr = -1;

  try {
    midiPort = new RtMidiIn();
  }
  catch (RtMidiError &error) {
    logger.FATAL("Unable to Initialize Midi: %s.", error.what());
  }

  devCount = midiPort->getPortCount();

  if (devCount == 0) {
    logger.FATAL("No midi device found.");
  }

  for (int i = 0; i < devCount; i++) {
    //cout << i << ": " << midiPort->getPortName(i) << endl;
    if ((devNbr == -1) &&
        (strcasestr(midiPort->getPortName(i).c_str(), config.midiDeviceName.c_str()) != NULL)) {
      devNbr = i;
    }
  }

  devNbr = config.midiDeviceNbr == -1 ? devNbr : config.midiDeviceNbr;

  return devNbr;
}

//---- Midi() ----

Midi::Midi()
{
  using namespace std;

  monitoring = false;
  sustainOn  = false;
  midiPort   = NULL;

  try {
    midiPort = new RtMidiIn();
  }
  catch (RtMidiError &error) {
    logger.FATAL("Unable to Initialize Midi: %s.", error.what());
  }

  int devNbr = findDeviceNbr();

  if (config.interactive) devNbr = selectDevice(devNbr);

  openPort(devNbr);

  try {
    midiPort->setCallback(&midiCallBack);
  }

  catch (RtMidiError &error) {
    logger.FATAL("Unable to set Midi CallBack: %s.", error.what());
  }

  if (config.midiChannel == -1) {
    logger.INFO("Listening to all MIDI channels.");
  }
  else {
    char data[60];
    char comma[2] = " ";
    char val[10];

    data[0] = 0;
    for (int i = 0; i < 16; i++) {
      if (config.midiChannel & (1 << i)) {
        sprintf(val, "%d", i + 1);
        strcat(data, comma);
        strcat(data, val);
        comma[0] = ',';
      }
    }
    logger.INFO("Listening to MIDI channels%s.", data);
  }

  channelMask = config.midiChannel;
}

//---- setNoteOn() ----

void Midi::setNoteOn(char note, char velocity)
{
  //DEBUG("Note ON %d (%d)\n", note, velocity);

  if (config.replayEnabled && (note == 108)) {
    sound->toggleReplay();
  }
  else {
    if (velocity == 0) {
      poly->noteOff(note, sustainOn);
    }
    else {
      soundFont->playNote(note, velocity);
    }
  }
}

//---- setNoteOff() ----

void Midi::setNoteOff(char note, char velocity)
{
  //DEBUG("Note OFF %d (%d)\n", note, velocity);

  (void) velocity;

  poly->noteOff(note, sustainOn);
}

//---- setSustainOn() ----

void Midi::setSustainOn()
{
  //DEBUG("Sustain ON\n");

  sustainOn = true;
}

//---- setSustainOff() ----

void Midi::setSustainOff()
{
  //DEBUG("Sustain OFF\n");

  if (sustainOn) {
    sustainOn = false;
    poly->voicesSustainOff();
  }
}

//---- openPort() ----

void Midi::openPort(int devNbr)
{
  if (midiPort->isPortOpen()) midiPort->closePort();

  try {
    midiPort->openPort(devNbr, "Mezzo Midi Port");
  }
  catch (RtMidiError &error) {
    logger.FATAL("Unable to open MIDI Device: %s.", error.what());
  }
}

//---- ~Midi() ----

Midi::~Midi()
{
  if (midiPort->isPortOpen()) midiPort->closePort();
  if (midiPort != NULL) delete midiPort;
}

//---- monitorMessages() ----

void Midi::monitorMessages()
{
    char ch;

    // Setup unbuffered nonechoed character input for stdin
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    new_tio.c_cc[VMIN]  = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    using namespace std;

    cout << endl << endl;
    cout << "MIDI MESSAGES MONITORING" << endl;
    cout << "Press any key to stop:" << endl << endl;

    monitoring = true;

    while (read(STDIN_FILENO, &ch, 1) == 0) {
      sleep(1);
    }
    cout << endl << endl;

    monitoring = false;

    // Restore buffered echoed character input for stdin
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

//---- interactiveAdjust() ----

void Midi::transposeAdjust()
{
  using namespace std;

  cout << endl << endl;

  int value;

  while (1) {
    cout << "Tranpose current value: " << config.midiTranspose << endl;
    cout << "Please enter New transpose value" << endl
         << "(as a number of semitone, between -24 and 24) > ";

    cin >> value;

    if ((value >= -24) && (value <= 24)) break;

    cout << "Value not valid. Please enter a value between -24 and 24." << endl << endl;
  }

  config.midiTranspose = value;
}
