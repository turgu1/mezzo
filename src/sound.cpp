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

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include <string.h>
#include <iomanip>

#include "mezzo.h"

#define show(v) std::cout << v << std::endl << std::flush

int soundCallback(void *               outputBuffer,
                  void *               inputBuffer,
                  unsigned int         nBufferFrames,
                  double               streamTime,
                  RtAudioStreamStatus  status,
                  void *               userData)
{
  static frameRecord buff;

  (void) inputBuffer; /* Prevent "unused variable" warnings. */
  (void) nBufferFrames;
  (void) userData;
  (void) streamTime;

  if (status) std::cout << "Stream RtAudio Underflow Detected." << std::endl;

  if (config.replayEnabled && sound->isReplaying()) {
    sound->get(buff);
  }
  else if (sound->holding()) {
    static frame_t zero = { 0.0f, 0.0f };
    std::fill(std::begin(buff), std::end(buff), zero);
    std::copy(std::begin(buff), std::end(buff), (frame_t *) outputBuffer);
  }
  else {
    poly->mixer(buff);
    reverb->process(buff);
    //equalizer->process(buff, nBufferFrames);
    //if (config.replayEnabled) sound->push(buff);

    Utils::clip((buffp) outputBuffer, buff);
  }

  //binFile.write((char *)buff, nBufferFrames * 8);

  return 0;
}

void Sound::openPort(int devNbr)
{
  RtAudio::StreamParameters params;

  if (dac.isStreamOpen()) dac.closeStream();

  params.deviceId     = devNbr;
  params.nChannels    = 2;
  params.firstChannel = 0;
  unsigned int bufferFrames = BUFFER_FRAME_COUNT;

  dac.showWarnings(false);

  try {
    dac.openStream(&params, NULL, RTAUDIO_FLOAT32,
                    config.samplingRate, &bufferFrames, &soundCallback);
    dac.startStream();

    RtAudio::DeviceInfo devInfo = dac.getDeviceInfo(devNbr);
    completeAudioPortName       = devInfo.name;
  }
  catch (RtAudioError& e) {
    e.printMessage();
  }
}

int Sound::findDeviceNbr()
{
  int devCount;

  RtAudio::DeviceInfo devInfo;
  int devNbr = -1;

  if ((devCount = dac.getDeviceCount()) >= 1) {

    if (!config.silent && !config.interactive) showDevices(devCount);

    if (config.pcmDeviceName.size() > 0) {
      for (int i = 0; i < devCount; i++) {
        devInfo = dac.getDeviceInfo(i);

        if ((devNbr == -1) && devInfo.probed &&
            (strcasestr(devInfo.name.c_str(), config.pcmDeviceName.c_str()) != NULL)) {
          devNbr = i;
          break;
        }
      }
    }

    if (devNbr == -1) {
      devNbr = dac.getDefaultOutputDevice();
    }
  }

  return devNbr;
}

Sound::Sound()
{
  setNewHandler(outOfMemory);

  using namespace std;

  replay = false;
  hold = true;

  int devNbr = findDeviceNbr();

  if (config.interactive) devNbr = selectDevice(devNbr);

  // Initialize replay buffer
  static frame_t zeroFrame = { 0.0f, 0.0f };
  std::fill(std::begin(replayBuffer), std::end(replayBuffer), zeroFrame);

  rhead = rtail = 0;

  openPort(devNbr);

  RtAudio::DeviceInfo devInfo = dac.getDeviceInfo(devNbr);
  if (!config.interactive) {
    logger.INFO("PCM Device selected (%d): %s.", devNbr, devInfo.name.c_str());
  }
}

Sound::~Sound()
{
  dac.abortStream();
  if (dac.isStreamOpen()) dac.closeStream();
}

void Sound::outOfMemory()
{
  logger.FATAL("Sound: Unable to allocate memory.");
}

void Sound::showDevices(int devCount)
{
  using namespace std;

  RtAudio::DeviceInfo devInfo;

  cout << endl << endl;
  cout << "PCM Device list:" << endl;
  cout << "---------------"  << endl;

  for (int i = 0; i < devCount; i++) {
    devInfo = dac.getDeviceInfo(i);

    if (devInfo.outputChannels > 0) {
      cout << "Device " << i << ": " << devInfo.name
           << ((devInfo.outputChannels > 0) ? " (out)" : " (in)")
           << endl;
    }
  }

  cout << "[End of list]" << endl << endl;
}

int Sound::selectDevice(int defaultNbr)
{
  using namespace std;

  int devCount;

  int devNbr = -1;

  if ((devCount = dac.getDeviceCount()) < 1) {
    logger.ERROR("No audio device found.");
  }

  showDevices(devCount);

  while (true) {
    string userData;
    int userNbr;

    cout << "Please enter PCM device number to use [" << defaultNbr << "]> ";

    getline(cin, userData);

    if (userData.length() == 0) {
      devNbr = defaultNbr;
      break;
    }

    userNbr = atoi(userData.c_str());

    if ((userNbr < 0) || (userNbr >= devCount)) {
      cout << "!! Invalid device number[" << userNbr << "]. Please try again !!" << endl;
    }
    else {
      devNbr = userNbr;
      break;
    }
  }
  cout << "PCM Device Selected: " << devNbr << endl;

  return devNbr;
}

//---- checkPort() ----
//
// This is called at interval of MONITOR_WAIT_COUNT seconds (see portMonitor() 
// in poly.cpp) to check if the midi controller is still available. 
// If not, it will then looks for it every second to 
// reconnect it with the rtMidi class once it is back on.

void Sound::checkPort()
{
  RtAudio::DeviceInfo devInfo;
  bool found = false;
  int devNbr;
  int devCount = dac.getDeviceCount();

  for (devNbr = 0; (devNbr < devCount) && !found; devNbr++) {
    devInfo = dac.getDeviceInfo(devNbr);
    found   = devInfo.probed && (devInfo.name == completeAudioPortName);
  }

  if (!found) {
    std::cout << "Audio Port Not Available." << std::endl;

    while (!found && keepRunning) {
      sleep(1);

      if (!keepRunning) return;

      devCount = dac.getDeviceCount();

      for (devNbr = 0; devNbr < devCount; devNbr++) {
        devInfo = dac.getDeviceInfo(devNbr);
        found   = devInfo.probed && (devInfo.name == completeAudioPortName);
        if (found) break;
      }
    }

    if (found) {
      std::cout << "Audio Port Is Back!" << std::endl;
      openPort(devNbr);
    }
  }
}
