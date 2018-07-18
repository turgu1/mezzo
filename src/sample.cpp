#include "copyright.h"

#include <cstring>
#include <string>
#include <iomanip>

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
  sampleRate     = info.dwSampleRate;
  pitch          = info.byOriginalPitch;
  correction     = info.chPitchCorrection;
  link           = info.wSampleLink;
  linkType       = info.sfSampleType;

  if ((info.dwEndloop - info.dwStartloop) >= 32) {
    startLoop    = info.dwStartloop - start;
    endLoop      = info.dwEndloop - start;
    sizeLoop     = endLoop - startLoop;
  }
  else {
    startLoop = endLoop = sizeLoop = 0;
  }

  sizeSample     = end - start;
  sizeFirstBlock = 0;

  assert(sizeSample > 0);
  assert(data != NULL); // Must have been set prior to instantiate Sample

  loaded = false;
}

Sample::~Sample()
{
  if (loaded && firstBlock) delete [] firstBlock;
  firstBlock = NULL;

  #if samples24bits
    if (firstBlock24) delete [] firstBlock24;
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
  //std::copy(firstBlock, firstBlock + sizeFirstBlock, &data[start]);

  #if samples24bits
    if (data24) {
      firstBlock24 = new int8_t[sizeFirstBlock];
      memcpy(firstBlock24, &data24[start], sizeFirstBlock);
      //std::copy(firstBlock24, firstBlock24 + sizeFirstBlock, &data24[start]);
    }
  #endif

  loaded = true;
  return true;
}

uint16_t Sample::getData(buffp buff, uint32_t pos, uint16_t qty, Synthesizer & synth)
{
  static int16_t tmpBuffer[SAMPLE_BUFFER_SAMPLE_COUNT];

  int16_t * iBuff = tmpBuffer;

  const uint32_t offset         = (synth.getStart() - start);
  const uint32_t sizeFirstBlock = synth.isLooping() ? MIN(this->sizeFirstBlock - offset, synth.getEndLoop()) :
                                                      this->sizeFirstBlock - offset;
  const uint32_t maxSampleSize  = synth.isLooping() ? synth.getEndLoop() :
                                                      synth.getSizeSample();
  uint16_t count = 0;

  // We will loop until a full buffer was returned or we are at the end
  // of the sample (no looping case)

  while (qty > 0) {

    uint16_t sizeToGet;
    
    if (pos >= maxSampleSize) {
      // The position has now passed the end of the sample.
      if (synth.isLooping()) {

        // We are looping. Find where we are in the loop portion
        uint32_t thePos = synth.getStartLoop() +
                          ((pos - maxSampleSize) % synth.getSizeLoop());
        sizeToGet = MIN(qty, maxSampleSize - thePos);

        // If we are still in the cached block, get some data from it
        if (thePos < sizeFirstBlock) {
          sizeToGet = MIN(sizeToGet, sizeFirstBlock - thePos);
          memcpy(iBuff, &firstBlock[offset + thePos], sizeToGet << 1);
        }
        else {
          //sizeToGet = MIN(sizeToGet, synth.getSizeSample() - thePos);
          memcpy(iBuff, &data[synth.getStart() + thePos], sizeToGet << 1);
        }
        assert((thePos + sizeToGet) <= maxSampleSize);
      }
      else {
        // We are not looping. Send back what was read.
        if (count > 0) Utils::shortToFloatNormalize(buff, tmpBuffer, count);
        return count;
      }
    }
    else if (pos < sizeFirstBlock) {
      sizeToGet = MIN(qty, sizeFirstBlock - pos);
      memcpy(iBuff, &firstBlock[offset + pos], sizeToGet << 1);
      assert((pos + sizeToGet) <= maxSampleSize);
    }
    else {
      sizeToGet = MIN(qty, maxSampleSize - pos);
      memcpy(iBuff, &data[synth.getStart() + pos], sizeToGet << 1);
      assert((pos + sizeToGet) <= maxSampleSize);
    }

    qty   -= sizeToGet;
    count += sizeToGet;
    iBuff += sizeToGet;
    pos   += sizeToGet;
  }

  Utils::shortToFloatNormalize(buff, tmpBuffer, count);

  assert(qty == 0);
  return count;
}

//---- showState() -----

void Sample::showStatus(int spaces)
{
  using namespace std;

  cout << setw(spaces) << ' '
       << "Sample:"
       << "[name:"       << name
       << " pitch:"      << +pitch
       << " start:"      << start
       << " end:"        << end
       << " startLoop:"  << startLoop
       << " endLoop:"    << endLoop
       << " size:"       << sizeSample
       << " rate:"       << sampleRate
       << " correction:" << +correction
       << " loaded:"     << (loaded ? "yes" : "no") 
       << "]" << endl;
}
