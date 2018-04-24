#include <cstring>

#include "mezzo.h"
#include "sample.h"

Sample::Sample(sfSample & info, uint16_t * dta, uint8_t * dta24)
{
  char theName[21];
  strncpy(theName, info.achSampleName, 20);
  theName[20]  = '\0';

  data         = dta;
  data24       = dta24;
  name         = theName;
  start        = info.dwStart;     
  end          = info.dwEnd;       
  startLoop    = info.dwStartloop;
  endLoop      = info.dwEndloop;
  sampleRate   = info.dwSampleRate;
  pitch        = info.byOriginalPitch;
  correction   = info.chPitchCorrection;
  link         = info.wSampleLink;
  linkType     = info.sfSampleType;
  
  firstBlock   = NULL;
  firstBlock24 = NULL;
}

Sample::~Sample()
{
  if (firstBlock  ) delete firstBlock;
  if (firstBlock24) delete firstBlock24;
  
  firstBlock   = NULL;
  firstBlock24 = NULL;
}

bool Sample::loadFirstBlock(uint32_t size) 
{
  blockSize    = size;
  firstBlock   = new uint16_t[size];
  firstBlock24 = new uint8_t[size];
  
  memcpy(firstBlock,   &data[start],   size * 2);
  memcpy(firstBlock24, &data24[start], size);
  
  return true;
}