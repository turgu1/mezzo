#include "copyright.h"

#ifndef _SAMPLE_
#define _SAMPLE_

#include "sf2.h"

typedef class Sample * samplep;

class Synthesizer;

class Sample  : public NewHandlerSupport<Sample> {

private:
  #if samples24bits
    static int8_t  * data24;
    int8_t         * firstBlock24;
  #endif

  static int16_t * data;     ///< Pointer on start of all samples data memory

  int16_t    * firstBlock;
  uint16_t     sizeFirstBlock;
  std::string  name;         ///< The name of the sample
  uint32_t     start;        ///< start offset in data
  uint32_t     end;          ///< end offset in data
  uint32_t     startLoop;
  uint32_t     endLoop;
  uint32_t     sampleRate;
  uint8_t      pitch;
  int8_t       correction;
  uint16_t     link;
  SFSampleLink linkType;
  uint32_t     sizeSample;
  uint32_t     sizeLoop;

  bool loaded;

  static void  outOfMemory();  ///< New operation handler when out of memory occurs

public:
  Sample(sfSample & info);
 ~Sample();

 /// Debugging method to show the current state of a sample
 void showStatus(int spaces);

  #if samples24bits
    static void setSamplesLocation(int16_t * dta, int8_t * dta24) { data = dta; data24 = dta24; };
  #else
    static void setSamplesLocation(int16_t * dta) { data = dta; };
  #endif

  bool load();

  /// This method returns data from the samples, managing the location
  /// (in the pre-loaded buffer or not) and the looping generation. The
  /// data is converted from its short int representation to a float
  /// with the value (-32768..32768) normalized to -1.0 .. 1.0
  /// Maximum size that can be loaded: 65535.

  uint16_t getData(sampleRecord & buff, uint32_t pos, Synthesizer & synth);

  /// Returns the name of the preset
  inline std::string &  getName() { return name; };
  inline uint8_t  getPitch()      { return pitch; };
  inline uint32_t getSampleRate() { return sampleRate; };
  inline uint32_t getStart()      { return start; };
  inline uint32_t getEnd()        { return end; };
  inline uint32_t getStartLoop()  { return startLoop; };
  inline uint32_t getEndLoop()    { return endLoop; };
  inline int8_t   getCorrection() { return correction; };
  //inline uint32_t get() { return ; };
};

#endif
