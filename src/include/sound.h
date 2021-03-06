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

#ifndef _SOUND_
#define _SOUND_

#include <portaudio.h>

/// The Sound class is responsible of supplying the proper linkage
/// with the PCM sound device. It uses the PortAudio library to
/// get access to the ALSA API of the operating system. Through the configuration file,
/// the user identify the PCM device to connect with. Once connected, the class setup
/// a callback function that will be asynchronously requesting new PCM data from the
/// application.
///
/// The class maintains a ~5 seconds replay buffer that could be requested to be played
/// by the user. It contains the last 5 seconds of PCM data generated and transmitted to
/// the PCM device and is used to debug samples libraries or code change in this program.
///
/// Getting the connection properly setup with the requested device is a somewhat difficult
/// exercise for the user. The class when started produce the list of compatible devices
/// and show the list to the end user. It is then possible for the user, in the interactive
/// mode, to select the targeted device and produce the data required in the configuration 
/// file to properly setup the Sound class.

#define CHKPA(stmt, msg) \
  if ((err = stmt) < 0) { logger.FATAL(msg, Pa_GetErrorText(err)); }

class Sound : public NewHandlerSupport<Sound> {

 private:
  PaStream *dac;  ///< The connection to the PortAudio stream
  
  #define REPLAY_FRAME_COUNT (860 * BUFFER_FRAME_COUNT) ///< Replay buffer size in frames

  bool hold;   ///< If true, stop asking form samples as a new library is being loaded
  bool replay; ///< Set to true is replaying is required by the user

  std::array<frame_t, 860 * BUFFER_FRAME_COUNT> replayBuffer; ///< FIFO Buffer on the last ~5 seconds PCM data
  uint32_t rhead; ///< Head Pointer on the replay buffer
  uint32_t rtail; ///< Tail Pointer on the replay buffer
  uint32_t rpos;  ///< Current replay position

  std::string completeAudioPortName;

  static void outOfMemory(); ///< New operation handler when out of memory occurs

  /// This is the callback function that will retrieve and process PCM data to be
  /// sent back to the PCM ALSA device. This function call the Poly::mixer method to get
  /// sampling data, then call the reverb and the equalizer filters in sequence to process
  /// the data before sending it to the PCM device. The data is also pushed in the FIFO
  /// replay buffer.
  ///
  /// This routine will be called by the RtAudio engine when audio is needed.
  /// It may be called at interrupt level on some machines so don't do anything
  /// that could mess up the system like calling memory allocation functions that would
  /// disrupt the data structure involved.

  friend int soundCallback(const void *                     inputBuffer,
                           void *                           outputBuffer,
                           unsigned long                    framesPerBuffer,
                           const PaStreamCallbackTimeInfo * timeInfo,
                           PaStreamCallbackFlags            statusFlags,
                           void *                           userData);
 public:
  /// This method selects which PCM device to connect to, establish the connection and
  /// setup the callback function.
   Sound();
  ~Sound();

  /// Open RtAudio stream
  void openPort(int devNbr);
  
  /// Show available devices to user
  void showDevices(int devCount);
  
  /// Find device number from configuration data
  int findDeviceNbr();

  /// Interactive device selection
  int selectDevice(int defaultNbr);

  // void checkPort(); // Not working.

  bool holding() { return hold; }

  /// Put sound on hold waiting for a new sample library to be loaded from disk
  void wait() {
    int err;

    hold = true;
    CHKPA(Pa_StopStream(dac), "Unable to stop PortAudio Stream: %s"); 
  }

  /// Restart sound, the new library has been loaded
  void conti() {
    int err;

    CHKPA(Pa_StartStream(dac), "Unable to start PortAudio Stream: %s"); 
    hold = false;
  }

  /// This is called when starting a replay process. The rpos pointer is adjusted to the
  /// tail of the FIFO buffer.
  inline void startReplay() { 
    rpos = rtail; 
  }

  /// Turn on and off the replay mechanism
  inline void toggleReplay() { 
    replay = !replay;
    if (replay) startReplay();
  }

  /// Returns true if the class is currently in a replay mode, false otherwise.
  inline bool isReplaying() { return replay; }

  /// Push in the replay buffer the just prepared PCM data. As the replay buffer is managed
  /// as a FIFO, it keep only the last 5 seconds of musical data.
  inline void push(frameRecord & b) {
    std::copy(std::begin(b), std::end(b), &replayBuffer[rhead]);
    rhead += b.size();
    if (rhead >= replayBuffer.size()) rhead = 0;
    rtail = rhead;
  }

  /// Retrieval of a chunk of PCM data from the replay buffer to send to the PCM device.
  inline void get(frameRecord & b) {
    std::copy(&replayBuffer[rpos], &replayBuffer[rpos + b.size()], std::begin(b));
    rpos += b.size();
    if (rpos >= replayBuffer.size()) rpos = 0;
  }
};

#endif
