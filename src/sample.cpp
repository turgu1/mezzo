#include "copyright.h"

#include <cstring>

#include "mezzo.h"
#include "sample.h"

uint16_t * Sample::data = NULL;

#if samples24bits
  uint16_t * Sample::data24 = NULL;
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

  sizeSample   = end - start;
  sizeLoop     = endLoop - startLoop;

  assert(sizeSample > 0);
  assert(data != NULL);
  
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
  
  uint32_t size = SAMPLE_BLOCK_SIZE;
  if (sizeSample < size) size = sizeSample;

  firstBlock = new uint16_t[size];
  memcpy(firstBlock, &data[start], size * 2);

  #if samples24bits
    if (data24) {
      firstBlock24 = new uint8_t[size];
      memcpy(firstBlock24, &data24[start], size);
    }
  #endif

  loaded = true;
  return true;
}

uint32_t Sample::getFirstBlock16(uint16_t * dta, bool loop)
{
  if (!loaded) return 0;
  
  if (sizeSample > SAMPLE_BLOCK_SIZE) {
    memcpy(dta, firstBlock, SAMPLE_BLOCK_SIZE * 2);
  }
  else {
    uint32_t i = sizeSample;
    memcpy(dta, firstBlock, sizeSample * 2);
    if ((sizeLoop == 0) || !loop) return sizeSample;
    while ((i + sizeLoop) < SAMPLE_BLOCK_SIZE) {
      memcpy(&dta[i], &data[startLoop], sizeLoop * 2);
      i += sizeLoop;
    }
    if (i < SAMPLE_BLOCK_SIZE) {
      memcpy(&dta[i], &data[startLoop], (SAMPLE_BLOCK_SIZE - i) * 2);
    }
  }

  return SAMPLE_BLOCK_SIZE;
}

// sizeSample = 21;
// sizeLoop = 7; loopStart = 14 loopEnd = 21
// bloclsize = 17;
//
// premier block [0]: 0..16
// deuxieme block [17]: 17..20 (4) 14..20 (7) 14..19 (6)
// troisieme block [34]: 20 (1) 14..20 (7) 14..20 (7) 14..15 (2)
// quatrieme [51]: 16..20 ....
//
// 34 - 21 = 13
// 13 mod 7 = 6
//
// 51 - 21 = 30
// 30 mod 7 = 2

uint32_t Sample::getBlockAtOffset16(uint32_t offset, uint16_t * dta, bool loop)
{
  if (!loaded) return 0;
  
  if (sizeSample > (offset + SAMPLE_BLOCK_SIZE)) {
    memcpy(dta, &data[start + offset], SAMPLE_BLOCK_SIZE * 2);
  }
  else {
    uint32_t i;
    if (offset < sizeSample) {
      i = sizeSample - offset;
      memcpy(dta, &data[start + offset], i * 2);
      if ((sizeLoop == 0) || !loop) return i;
    }
    else {
      if ((sizeLoop == 0) || !loop) return 0;
      i = sizeLoop - ((offset - sizeSample) % sizeLoop);
      if (i > SAMPLE_BLOCK_SIZE) i = SAMPLE_BLOCK_SIZE;
      memcpy(dta, &data[endLoop - i], i * 2);
    }
    while ((i + sizeLoop) < SAMPLE_BLOCK_SIZE) {
      memcpy(&dta[i], &data[startLoop], sizeLoop * 2);
      i += sizeLoop;
    }
    if (i < SAMPLE_BLOCK_SIZE) {
      memcpy(&dta[i], &data[startLoop], (SAMPLE_BLOCK_SIZE - i) * 2);
    }
  }

  return SAMPLE_BLOCK_SIZE;
}
