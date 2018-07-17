#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include <string.h>
#include <iomanip>

#include "mezzo.h"
#include "sound.h"

#define CHKPA(stmt, msg) \
  if ((err = stmt) < 0) { logger.FATAL(msg, Pa_GetErrorText(err)); }

int soundCallback(const void *                     inputBuffer,
                  void *                           outputBuffer,
                  unsigned long                    framesPerBuffer,
                  const PaStreamCallbackTimeInfo * timeInfo,
                  PaStreamCallbackFlags            statusFlags,
                  void *                           userData)
{
  buffp buff = (buffp) outputBuffer;

  (void) inputBuffer; /* Prevent "unused variable" warnings. */
  (void) userData;
  (void) timeInfo;
  (void) statusFlags;

  if (config.replayEnabled && sound->isReplaying()) {
    sound->get(buff);
  }
  else if (sound->holding()) {
    std::fill(buff, buff + framesPerBuffer + framesPerBuffer, 0.0f);
  }
  else {
    poly->mixer(buff, framesPerBuffer);
    //reverb->process(buff, framesPerBuffer);
    //equalizer->process(buff, framesPerBuffer);
    //if (config.replayEnabled) sound->push(buff);
  }

  //binFile.write((char *)buff, framesPerBuffer * 8);

  return paContinue;
}

void Sound::openPort(int devNbr)
{
  int err;

  PaStreamParameters params;
  PaStreamFlags      flags;

  params.device       = devNbr;
  params.channelCount = 2;

  params.sampleFormat = paFloat32;
  flags = 0;

  params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
  params.hostApiSpecificStreamInfo = NULL;

  CHKPA(Pa_OpenStream(&stream,
                      NULL, &params, config.samplingRate,
                      BUFFER_FRAME_COUNT, flags, &soundCallback, NULL),
        "Unable to open PortAudio Stream: %s");

  CHKPA(Pa_StartStream(stream),
        "Unable to start PortAudio Stream: %s");
}

int Sound::findDeviceNbr()
{
  int devCount;

  const PaDeviceInfo *devInfo;
  int devNbr = config.pcmDeviceNbr;;

  if ((devCount = Pa_GetDeviceCount()) < 0) {
    logger.FATAL("Unable to get audio device count: %s.", 
                 Pa_GetErrorText(devCount));
  }

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
    devNbr =  Pa_GetDefaultOutputDevice();
    if (devNbr == paNoDevice) devNbr = 0;
    logger.INFO("Default PCM Device (%d) selected.", devNbr);
  }
  else {
    logger.INFO("PCM Device Selected: %d.", devNbr);
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

  stream = NULL;

  CHKPA(Pa_Initialize(), "Unable to initialize PortAudio: %s");

  int devNbr = findDeviceNbr();
  
  if (config.interactive) devNbr = selectDevice(devNbr);

  openPort(devNbr);

  // Initialize replay buffer
  rbuff = new sample_t[REPLAY_BUFFER_SAMPLE_COUNT];

  std::fill(rbuff, rbuff + REPLAY_BUFFER_SAMPLE_COUNT, 0.0f);

  rhead = rtail = rbuff;
  rend  = &rbuff[REPLAY_BUFFER_SAMPLE_COUNT];
}

Sound::~Sound()
{
  if (stream) {
    Pa_AbortStream(stream);
    Pa_CloseStream(stream);
  }

  delete [] rbuff;

  Pa_Terminate();
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

    assert(devInfo != NULL);

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

  if ((devCount = Pa_GetDeviceCount()) < 0) {
    logger.ERROR("Unable to get audio device count: %s.", Pa_GetErrorText(devCount));
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
