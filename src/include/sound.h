#include "copyright.h"

#ifndef _SOUND_
#define _SOUND_

#ifdef __APPLE__
  #include "RtAudio.h"
#else
  #include "rtaudio/RtAudio.h"
#endif

/// The Sound class is responsible of supplying the proper linkage
/// with the PCM sound device. It uses the RtAudio library to
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

class Sound : public NewHandlerSupport<Sound> {

 private:
  RtAudio dac;  ///< The connection to the RtAudio stream
  
  #define REPLAY_FRAME_COUNT (860 * BUFFER_FRAME_COUNT) ///< Replay buffer size in frames

  bool hold;   ///< If true, stop asking form samples as a new library is being loaded
  bool replay; ///< Set to true is replaying is required by the user

  std::array<frame_t, 860 * BUFFER_FRAME_COUNT> replayBuffer; ///< FIFO Buffer on the last ~5 seconds PCM data
  uint32_t rhead; ///< Head Pointer on the replay buffer
  uint32_t rtail; ///< Tail Pointer on the replay buffer
  uint32_t rpos;  ///< Current replay position

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

  friend int soundCallback(void *              outputBuffer,
                           void *              inputBuffer,
                           unsigned int        nBufferFrames,
                           double              streamTime,
                           RtAudioStreamStatus status,
                           void *              userData);
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

  bool holding() { return hold; }

  /// Put sound on hold waiting for a new sample library to be loaded from disk
  void wait() { hold = true; }

  /// Restart sound, the new library has been loaded
  void conti() { hold = false; }

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
