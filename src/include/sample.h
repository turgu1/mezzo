#include "copyright.h"

#ifndef _SAMPLE_
#define _SAMPLE_

#include "sf2.h"

class Sample  : public NewHandlerSupport<Sample> {
  
private:
  #if samples24bits
    static uint8_t  * data24;
    uint8_t         * firstBlock24;
  #endif

  static uint16_t * data;    ///< Pointer on start of all samples data memory

  uint16_t   * firstBlock;
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

  #if samples24bits
    static void setSamplesLocation(uint16_t * dta, uint8_t * dta24) { data = dta; data24 = dta24; };
  #else
    static void setSamplesLocation(uint16_t * dta) { data = dta; };
  #endif

  bool load();

  uint32_t getFirstBlock16(uint16_t * dta, bool loop);
  uint32_t getBlockAtOffset16(uint32_t offset, uint16_t * dta, bool loop);

  /// Returns the name of the preset
  std::string & getName() { return name; };
  uint32_t getSampleRate() { return sampleRate; };
};

#endif
