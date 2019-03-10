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

  #if loadInMemory
    samples = NULL;
  #else
    firstBlock     = NULL;
    #if samples24bits
      firstBlock24 = NULL;
    #endif
    sizeFirstBlock = 0;
  #endif

  name           = theName;
  start          = info.dwStart;
  end            = info.dwEnd;
  sampleRate     = info.dwSampleRate;
  pitch          = info.byOriginalPitch;
  correction     = info.chPitchCorrection;
  link           = info.wSampleLink;
  linkType       = info.sfSampleType;

//  if ((info.dwEndloop - info.dwStartloop) >= 32) {
    startLoop    = info.dwStartloop - start;
    endLoop      = info.dwEndloop - start;
    sizeLoop     = endLoop - startLoop;
//  }
//  else {
//    startLoop = endLoop = sizeLoop = 0;
//  }

  sizeSample     = end - start;

  assert(sizeSample > 0);
  assert(data != NULL); // Must have been set prior to instantiate Sample

  loaded = false;
}

Sample::~Sample()
{
  #if loadInMemory
    if (loaded && samples) delete [] samples;
    samples = NULL;
  #else
    if (loaded && firstBlock) delete [] firstBlock;
    firstBlock = NULL;

    #if samples24bits
      if (firstBlock24) delete [] firstBlock24;
      firstBlock24 = NULL;
    #endif
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

  #if loadInMemory
    samples = new sample_t[sizeSample];
    Utils::shortToFloatNormalize(samples, &data[start], sizeSample);
  #else
    sizeFirstBlock = SAMPLE_BLOCK_SIZE;
    if (sizeSample < sizeFirstBlock) sizeFirstBlock = sizeSample;

    firstBlock = new int16_t[sizeFirstBlock];
    std::copy(&data[start], &data[start + sizeFirstBlock], firstBlock);

    #if samples24bits
      if (data24) {
        firstBlock24 = new int8_t[sizeFirstBlock];
        std::copy(&data24[start], &data24[start + sizeFirstBlock], firstBlock24);
      }
    #endif
  #endif

  loaded = true;
  return true;
}

// static bool first = true;
// static bool getIt = false;
// static Sample * theSample = NULL;
// const uint32_t thePos = 256 * 10;

uint16_t Sample::getData(sampleRecord & buff, uint32_t pos, Synthesizer & synth)
{
  // if (first) {
  //   theSample = this;
  //   first = false;
  // }

  // if ((theSample == this) && (pos == thePos)) {
  //   getIt = true;
  // }

  uint16_t qty = BUFFER_SAMPLE_COUNT;

  #if loadInMemory
    sample_t * iBuff = &buff[0];
    uint16_t   count = 0;

    const uint32_t offset  = (synth.getStart() - start);
    const uint32_t size    = synth.isLooping() ? MIN(sizeSample - offset, synth.getEndLoop()) :
                                                 sizeSample - offset;
    const uint32_t maxSize = synth.isLooping() ? synth.getEndLoop() :
                                                 synth.getSizeSample();

    // We will loop until a full buffer was returned or we are at the end
    // of the sample (no looping case)

    while (qty > 0) {

      uint16_t sizeToGet;

      if (pos >= maxSize) {
        // The position has now passed the end of the sample.
        if (synth.isLooping()) {

          // We are looping. Find where we are in the loop portion
          uint32_t thePos = synth.getStartLoop() +
                            ((pos - maxSize) % synth.getSizeLoop());
          sizeToGet = MIN(qty, maxSize - thePos);
          sizeToGet = MIN(sizeToGet, size - thePos);
          std::copy(&samples[offset + thePos], &samples[offset + thePos + sizeToGet], iBuff);
          assert((thePos + sizeToGet) <= maxSize);
        }
        else {
          // We are not looping. Send back what was read with the rest as lastValue.
          sizeToGet = qty;
          std::fill(iBuff, iBuff + sizeToGet, synth.getLastValue());
        }
      }
      else {
        sizeToGet = MIN(qty, size - pos);
        assert((pos + sizeToGet) <= maxSize);
        std::copy(&samples[offset + pos], &samples[offset + pos + sizeToGet], iBuff);
      }

      qty   -= sizeToGet;
      count += sizeToGet;
      iBuff += sizeToGet;
      pos   += sizeToGet;
    }
    synth.setLastValue(buff[count - 1]);
  #else
    rawSampleRecord tmpBuffer;

    int16_t * iBuff = &tmpBuffer[0];
    uint16_t  count = 0;

    const uint32_t offset         = (synth.getStart() - start);
    const uint32_t sizeFirstBlock = synth.isLooping() ? MIN(this->sizeFirstBlock - offset, synth.getEndLoop()) :
                                                        this->sizeFirstBlock - offset;
    const uint32_t maxSampleSize  = synth.isLooping() ? synth.getEndLoop() :
                                                        synth.getSizeSample();
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
            std::copy(&firstBlock[offset + thePos], &firstBlock[offset + thePos + sizeToGet], iBuff);
          }
          else {
            std::copy(&data[synth.getStart() + thePos], &data[synth.getStart() + thePos + sizeToGet], iBuff);
          }
          assert((thePos + sizeToGet) <= maxSampleSize);
        }
        else {
          // We are not looping. Send back what was read with the rest as lastValue.
          sizeToGet = qty;
          std::fill(iBuff, iBuff + sizeToGet, synth.getLastValue());
        }
      }
      else if (pos < sizeFirstBlock) {
        sizeToGet = MIN(qty, sizeFirstBlock - pos);
        assert((pos + sizeToGet) <= maxSampleSize);
        std::copy(&firstBlock[offset + pos], &firstBlock[offset + pos + sizeToGet], iBuff);
      }
      else {
        sizeToGet = MIN(qty, maxSampleSize - pos);
        assert((pos + sizeToGet) <= maxSampleSize);
        std::copy(&data[synth.getStart() + pos], &data[synth.getStart() + pos + sizeToGet], iBuff);
      }

      qty   -= sizeToGet;
      count += sizeToGet;
      iBuff += sizeToGet;
      pos   += sizeToGet;
    }

    synth.setLastValue(tmpBuffer[count - 1]);

    Utils::shortToFloatNormalize(buff, tmpBuffer, count);
  #endif

  // if (getIt) {
  //   getIt = false;
  //   binFile.write((const char *) &buff, count * 4);
  // }

  assert(qty == 0);
  return count;
}

buffp Sample::getData2(uint32_t & count, uint32_t pos, Synthesizer & synth)
{
  const uint32_t offset  = (synth.getStart() - start);
  const uint32_t size    = synth.isLooping() ? MIN(sizeSample - offset, synth.getEndLoop()) :
                                               sizeSample - offset;
  const uint32_t maxSize = synth.isLooping() ? synth.getEndLoop() :
                                               synth.getSizeSample();

  if (pos >= maxSize) {
    // The position has now passed the end of the sample.
    if (synth.isLooping()) {

      // We are looping. Find where we are in the loop portion
      uint32_t thePos = synth.getStartLoop() +
                        ((pos - maxSize) % synth.getSizeLoop());
      count = MIN(maxSize - thePos, size - thePos);
      return &samples[offset + thePos];
    }
    else {
      count = 0;
      return samples;
    }
  }
  else {
    count = size - pos;
    return &samples[offset + pos];
  }
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
