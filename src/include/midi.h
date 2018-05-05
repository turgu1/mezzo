#include "copyright.h"

#ifndef _MIDI_
#define _MIDI_

#ifdef __APPLE__
  #include "RtMidi.h"
#else
  #include "rtmidi/RtMidi.h"
#endif

class Midi {

 private:
  /// This is the callback method called by RtMidi to signify the reception of a new MIDI command
  /// by the application. This callback is responsible of parsing the command and dispatch to modify
  /// Mezzo voices state accordingly.
  friend void midiCallBack (double timeStamp,
                            std::vector<unsigned char> *message,
                            void *userData);
 public:
   Midi();
  ~Midi();
  void monitorMessages();
  void transposeAdjust(); ///< Interactive tranpose value gathering
  
  /// Find device number from configuration data
  int findDeviceNbr();
  
  int selectDevice(int defaultNbr);
  void openPort(int devNbr);

 private:
  RtMidiIn * midiPort;  ///< RTMidiIn instance
  bool monitoring;      ///< True if monitoring midi in interactive mode
  bool sustainOn;       ///< True if the sustain pedal is depressed by the user
  int  channelMask;     ///< Mask of channels being listened by Midi

  void showDevices(int devCount);

  void setNoteOn(char note, char velocity);  ///< Process a noteOn MIDI command
  void setNoteOff(char note, char velocity); ///< Process a noteOff MIDI command
  void setSustainOn();
  void setSustainOff();

  bool sustainIsOn() { return sustainOn; };
};

#endif
