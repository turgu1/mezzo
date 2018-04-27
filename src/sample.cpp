#include "copyright.h"

#include <cstring>
#include <string>

#include "mezzo.h"

int16_t * Sample::data = NULL;

#if samples24bits
  int8_t * Sample::data24 = NULL;
#endif

Sample::Sample(sfSample & info)
{
  setNewHandler(outOfMemory);

  char theName[21];
  strncpy(theName, info.achSampleName, 20);
  theName[20]  = '\0';

  firstBlock     = NULL;
  #if samples24bits
    firstBlock24 = NULL;
  #endif

  name           = theName;
  start          = info.dwStart;
  end            = info.dwEnd;
  startLoop      = info.dwStartloop - start;
  endLoop        = info.dwEndloop - start;
  sampleRate     = info.dwSampleRate;
  pitch          = info.byOriginalPitch;
  correction     = info.chPitchCorrection;
  link           = info.wSampleLink;
  linkType       = info.sfSampleType;

  sizeSample     = end - start;
  sizeLoop       = endLoop - startLoop;
  sizeFirstBlock = 0;

  assert(sizeSample > 0);
  assert(data != NULL); // Must have been set prior to instantiate Sample
  
  loaded = false;
}

Sample::~Sample()
{
  if (firstBlock) delete firstBlock;
  firstBlock = NULL;

  #if samples24bits
    if (firstBlock24) delete firstBlock24;
    firstBlock24 = NULL;
  #endif
  
  loaded = false;
}

void Sample::outOfMemory()
{
  logger.FATAL("Sample: Unable to allocate memory.");
}

bool Sample::load()
{
  if (loaded) return true;
  
  sizeFirstBlock = SAMPLE_BLOCK_SIZE;
  if (sizeSample < sizeFirstBlock) sizeFirstBlock = sizeSample;

  firstBlock = new int16_t[sizeFirstBlock];
  memcpy(firstBlock, &data[start], sizeFirstBlock * 2);

  #if samples24bits
    if (data24) {
      firstBlock24 = new int8_t[sizeFirstBlock];
      memcpy(firstBlock24, &data24[start], sizeFirstBlock);
    }
  #endif

  loaded = true;
  return true;
}

uint16_t Sample::getData(buffp buff, uint32_t pos, uint16_t qty, bool loop)
{
  uint16_t count = 0;
  
  while (qty > 0) {
    uint16_t size;
    if (pos >= sizeSample) {
      if ((sizeLoop > 0) && loop) {
        pos = startLoop + (pos - sizeSample) % sizeLoop;
        size = MIN(qty, endLoop - pos);
      }
      else return count;
    }
    if (pos < sizeFirstBlock) {
      size = MIN(qty, sizeFirstBlock - pos);
      shortToFloatNormalize(buff, &firstBlock[pos], size);
    }
    else {
      size = MIN(qty, sizeSample - pos);
      shortToFloatNormalize(buff, &data[start + pos], size);
    }
    qty   -= size;
    count += size;
    buff  += size;
  }
  
  assert(qty == 0);
  return count;
}

//---- showState() -----

void Sample::showState()
{
  using namespace std;

  cout << "sample: "
       << "name:"       << name       << " "
       << "pitch:"      << pitch      << " "
       << "start:"      << start      << " "
       << "end:"        << end        << " "
       << "startLoop:"  << startLoop  << " "
       << "endLoop:"    << endLoop    << " "
       << "size:"       << sizeSample << " "
       << "loaded:"     << (loaded ? "yes" : "no")   << endl;
}
