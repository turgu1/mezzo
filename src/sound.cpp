#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include <string.h>
#include <iomanip>

#include "mezzo.h"
#include "sound.h"


int soundCallback(void *               outputBuffer,
                  void *               inputBuffer,
                  unsigned int         nBufferFrames,
                  double               streamTime,
                  RtAudioStreamStatus  status,
                  void *               userData)
{
  buffp buff = (buffp) outputBuffer;

  (void) inputBuffer; /* Prevent "unused variable" warnings. */
  (void) userData;
  (void) streamTime;

  if (status) std::cout << "Stream RtAudio Underflow Detected." << std::endl;

  if (config.replayEnabled && sound->isReplaying()) {
    sound->get(buff);
  }
  else if (sound->holding()) {
    std::fill(buff, buff + nBufferFrames + nBufferFrames, 0.0f);
  }
  else {
    poly->mixer(buff, nBufferFrames);
    reverb->process(buff, nBufferFrames);
    //equalizer->process(buff, nBufferFrames);
    //if (config.replayEnabled) sound->push(buff);
  }

  //binFile.write((char *)buff, nBufferFrames * 8);

  return 0;
}

void Sound::openPort(int devNbr)
{
  RtAudio::StreamParameters params;

  params.deviceId     = devNbr;
  params.nChannels    = 2;
  params.firstChannel = 0;
  unsigned int bufferFrames = BUFFER_FRAME_COUNT;

  dac.showWarnings(false);

  try {
    dac.openStream(&params, NULL, RTAUDIO_FLOAT32,
                    config.samplingRate, &bufferFrames, &soundCallback);
    dac.startStream();
  }
  catch (RtAudioError& e) {
    e.printMessage();
  }
}

int Sound::findDeviceNbr()
{
  int devCount;

  RtAudio::DeviceInfo devInfo;
  int devNbr = config.pcmDeviceNbr;

  if ((devCount = dac.getDeviceCount()) < 1) {
    logger.FATAL("No audio device found");
  }

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
    logger.INFO("Default PCM Device (%d) selected.", devNbr);
  }
  else {
    logger.INFO("PCM Device Selected: %d.", devNbr);
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
  rbuff = new sample_t[REPLAY_BUFFER_SAMPLE_COUNT];

  std::fill(rbuff, rbuff + REPLAY_BUFFER_SAMPLE_COUNT, 0.0f);

  rhead = rtail = rbuff;
  rend  = &rbuff[REPLAY_BUFFER_SAMPLE_COUNT];

  openPort(devNbr);
}

Sound::~Sound()
{
  dac.abortStream();
  if (dac.isStreamOpen()) dac.closeStream();

  delete [] rbuff;
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
