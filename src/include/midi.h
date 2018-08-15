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

  void checkPort();

 private:
  RtMidiIn * midiPort;  ///< RTMidiIn instance
  std::string completeMidiPortName;
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
