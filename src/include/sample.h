#ifndef _SAMPLE_
#define _SAMPLE_

#include "sf2.h"

class Sample
{
private:
  uint16_t   * data;
  uint8_t    * data24;
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
  
  uint16_t * firstBlock;
  uint8_t  * firstBlock24;
  uint32_t   blockSize;
   
public:
  Sample(sfSample & info, uint16_t * dta, uint8_t * dta24);
 ~Sample();
  
  bool loadFirstBlock(uint32_t size);
  
  bool getFirstBlock(uint16_t ** dta, uint8_t ** dta24) {
    *dta   = firstBlock;
    *dta24 = firstBlock24;
    return true;
  };
  
  bool getBlockAtOffset(uint32_t offset, uint16_t ** dta, uint8_t ** dta24) {
    *dta   = &data[start + offset];
    *dta24 = &data24[start + offset];
    return true;
  };
  
  /// Returns the name of the preset
  std::string & getName() { return name; }
};

#endif