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

int soundCallback(const void *                     inputBuffer,
                  void *                           outputBuffer,
                  unsigned long                    framesPerBuffer,
                  const PaStreamCallbackTimeInfo * timeInfo,
                  PaStreamCallbackFlags            statusFlags,
                  void *                           userData)
{
  static frameRecord buff;

  (void) inputBuffer; /* Prevent "unused variable" warnings. */
  (void) framesPerBuffer;
  (void) userData;
  (void) timeInfo;

  if ((statusFlags & paOutputUnderflow) && !sound->holding()) std::cout << "PortAudio Stream Underflow Detected." << std::endl;

  if (config.replayEnabled && sound->isReplaying()) {
    sound->get(buff);
    binFile.write((char *)buff.data(), framesPerBuffer * 8);
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
    metronome->process(buff);
    if (config.replayEnabled) sound->push(buff);
  }
  
  Utils::clip((float *) outputBuffer, buff);

  return 0;
}

void Sound::openPort(int devNbr)
{
  int err;

  PaStreamParameters params;
  PaStreamFlags      flags;

  if ((dac != NULL) && Pa_IsStreamActive(dac)) {
    CHKPA(Pa_CloseStream(dac), "Unable to close PortAudio stream: %s");
  }

  params.device           = devNbr;
  params.channelCount     = 2;
  params.sampleFormat     = paFloat32;
  params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
  params.hostApiSpecificStreamInfo = NULL;

  flags = 0;

  CHKPA(Pa_OpenStream(&dac,
                      NULL, &params, config.samplingRate,
                      BUFFER_FRAME_COUNT, flags, &soundCallback, NULL),
        "Unable to open PortAudio Stream: %s");

  const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(devNbr);
  completeAudioPortName.assign(devInfo->name);

  // std::cout << "Complete Audio Port Name: " << completeAudioPortName << std::endl << std::flush;

}

int Sound::findDeviceNbr()
{
  int devCount;

  const PaDeviceInfo *devInfo;
  int devNbr = -1;

  if ((devCount = Pa_GetDeviceCount()) >= 1) {

    if (!config.silent && !config.interactive) showDevices(devCount);

    if (config.pcmDeviceName.size() > 0) {
      for (int i = 0; i < devCount; i++) {
        devInfo = Pa_GetDeviceInfo(i);

        if ((devNbr == -1) &&
            (strcasestr(devInfo->name, config.pcmDeviceName.c_str()) != NULL)) {
          devNbr = i;
          break;
        }
      }
    }

    if (devNbr == -1) {
      devNbr = Pa_GetDefaultOutputDevice();
    }
  }

  return devNbr;
}

Sound::Sound()
{
  int err;

  setNewHandler(outOfMemory);

  using namespace std;

  replay = false;
  hold = true;

  dac = NULL;

  CHKPA(Pa_Initialize(), "Unable to initialize PortAudio: %s");

  std::cout << Pa_GetVersionText() << std::endl;

  int devNbr = findDeviceNbr();

  if (config.interactive) devNbr = selectDevice(devNbr);

  // Initialize replay buffer
  static frame_t zeroFrame = { 0.0f, 0.0f };
  std::fill(std::begin(replayBuffer), std::end(replayBuffer), zeroFrame);

  rhead = rtail = 0;

  openPort(devNbr);

  if (!config.interactive) {
    logger.INFO("PCM Device selected (%d): %s.", devNbr, completeAudioPortName.c_str());
  }
}

Sound::~Sound()
{
  int err;

  if (dac != NULL) {
    CHKPA(Pa_CloseStream(dac), "Unable to close PortAudio stream: %s");
  }
  CHKPA(Pa_Terminate(), "Unable to terminate PortAudio: %s");
}

void Sound::outOfMemory()
{
  logger.FATAL("Sound: Unable to allocate memory.");
}

void Sound::showDevices(int devCount)
{
  using namespace std;

  const PaDeviceInfo *devInfo;

  cout << endl << endl;
  cout << "PCM Device list:" << endl;
  cout << "---------------"  << endl;

  for (int i = 0; i < devCount; i++) {
    devInfo = Pa_GetDeviceInfo(i);

    if (devInfo->maxOutputChannels > 0) {
      cout << "Device " << i << ": " << devInfo->name
           << ((devInfo->maxOutputChannels > 0) ? " (out)" : " (in)")
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

  if ((devCount = Pa_GetDeviceCount()) < 1) {
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
// This code is *not* in a working condition as PortAudio lack the HotPlug mechanism.
//  
// This is called at interval of MONITOR_WAIT_COUNT seconds (see portMonitor() 
// in poly.cpp) to check if the midi controller is still available. 
// If not, it will then looks for it every second to 
// reconnect it with the rtMidi class once it is back on.
//
// This is working fine under OSX (CoreAudio). Under Linux ALSA, the
// RtAudio library doen't behave properly as we can't discover that the
// device was disconnected (getDeviceInfo() returns a cached structure).

#if 0
void Sound::checkPort()
{
  const PaDeviceInfo *devInfo;
  bool found = false;
  int devNbr;
  int devCount = Pa_GetDeviceCount();

  //std::cout << "Audio Device Count: " << devCount << std::endl;

  std::cout << "Stream active? " << (Pa_IsStreamActive(dac) ? "YES" : "NO") << std::endl;

  for (devNbr = 0; (devNbr < devCount) && !found; devNbr++) {
    devInfo = Pa_GetDeviceInfo(devNbr);
    found   = completeAudioPortName.compare(devInfo->name) == 0;
    std::cout << "[" << devInfo->name << "] vs [" << completeAudioPortName << "] found: " << (found ? "YES" : "NO") << std::endl;
  }

  if (!found) {
    wait();

    std::cout << "Audio Port Not Available." << std::endl;

    while (!found && keepRunning) {
      sleep(1);

      if (!keepRunning) return;

      devCount = Pa_GetDeviceCount();

      for (devNbr = 0; devNbr < devCount; devNbr++) {
        devInfo = Pa_GetDeviceInfo(devNbr);
        found   = (completeAudioPortName.compare(devInfo->name) == 0);
        if (found) break;
      }
    }

    if (found) {
      std::cout << "Audio Port Is Back!" << std::endl;
      openPort(devNbr);
    }
  }
}
#endif