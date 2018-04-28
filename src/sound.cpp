#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include <string.h>
#include <iomanip>

#include "mezzo.h"
#include "sound.h"

//---- soundCallback() ----

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
    memset(buff, 0, framesPerBuffer << LOG_FRAME_SIZE);
  }
  else {
    poly->mixer(buff, framesPerBuffer);
    reverb->process(buff, framesPerBuffer);
    equalizer->process(buff, framesPerBuffer);
    if (config.replayEnabled) sound->push(buff);
  }

  return paContinue;
}

//---- Sound() ----

#define CHKPA(stmt, msg) \
  if ((err = stmt) < 0) { logger.FATAL(msg, Pa_GetErrorText(err)); }

Sound::Sound()
{
  int err;

  setNewHandler(outOfMemory);

  using namespace std;

  replay = false;
  hold = true;

  stream = NULL;

  CHKPA(Pa_Initialize(), "Unable to initialize PortAudio: %s");

  if (config.interactive) {
    selectDevice();
  }
  else {
    int devCount;

    PaStreamParameters params;
    PaStreamFlags      flags;

    const PaDeviceInfo *devInfo;
    int devNbr = -1;

    if ((devCount = Pa_GetDeviceCount()) < 0) {
      logger.FATAL("Unable to get audio device count: %s.", Pa_GetErrorText(devCount));
    }

    if (!config.silent) showDevices(devCount);

    if (config.pcmDeviceNbr != -1) {
      devNbr : config.pcmDeviceNbr;
    }
    else if (config.pcmDeviceName.size() > 0) {
      for (int i = 0; i < devCount; i++) {
        devInfo = Pa_GetDeviceInfo(i);

        if ((devNbr == -1) && (strcasestr(devInfo->name, config.pcmDeviceName.c_str()) != NULL)) {
          devNbr = i;
          break;
        }
      }
    }

    if (devNbr == -1) {
      devNbr = 0;
      logger.INFO("Default PCM Device (0) selected.");
    }
    else {
      logger.INFO("PCM Device Selected: %d.", devNbr);
    }

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

  // Initialize replay buffer
  rbuff = new sample_t[REPLAY_BUFFER_SAMPLE_COUNT];

  memset(rbuff, 0, REPLAY_BUFFER_SIZE);

  rhead = rtail = rbuff;
  rend  = &rbuff[REPLAY_BUFFER_SAMPLE_COUNT];
}

//---- ~Sound() ----

Sound::~Sound()
{
  if (stream) {
    Pa_AbortStream(stream);
    Pa_CloseStream(stream);
  }

  delete [] rbuff;

  Pa_Terminate();
}

//----- outOfMemory() ----

void Sound::outOfMemory()
{
  logger.FATAL("Sound: Unable to allocate memory.");
}

//---- showDevices() ----

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

    cout << "Device " << i << ": " << devInfo->name << endl;
  }

  cout << "[End of list]" << endl << endl;
}

//---- selectDevice() ----

void Sound::selectDevice()
{
  using namespace std;

  int devCount;
  int err;

  PaStreamParameters params;
  PaStreamFlags      flags;

  int devNbr = -1;

  if ((devCount = Pa_GetDeviceCount()) < 0) {
    logger.ERROR("Unable to get audio device count: %s.", Pa_GetErrorText(devCount));
  }

  showDevices(devCount);

  while (true) {
    char userData[6];
    int userNbr;

    cout << "Please enter PCM device number to use > ";
    cin >> setw(5) >> userData;

    userNbr = atoi(userData);

    if ((userNbr < 0) || (userNbr >= devCount)) {
      cout << "!! Invalid device number[" << userNbr << "]. Please try again !!" << endl;
    }
    else {
      devNbr = userNbr;
      break;
    }
  }
  cout << "PCM Device Selected: " << devNbr << endl << endl;

  params.device       = devNbr;
  params.channelCount = 2;

  params.sampleFormat = paFloat32;
  flags = 0;

  params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
  params.hostApiSpecificStreamInfo = NULL;

  if (stream != NULL) {
    Pa_AbortStream(stream);
    Pa_CloseStream(stream);
  }

  CHKPA(Pa_OpenStream(&stream,
                      NULL, &params, config.samplingRate,
                      BUFFER_FRAME_COUNT, flags, &soundCallback, NULL),
        "Unable to open PortAudio Stream: %s");

  CHKPA(Pa_StartStream(stream),
        "Unable to start PortAudio Stream: %s");
}
