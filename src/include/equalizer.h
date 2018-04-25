#include "copyright.h"

#ifndef _EQUALIZER_
#define _EQUALIZER_

// A very simple form of a 7 bands equalizer. 
// IIR Filter algorithm and equalizer coefficients from the following book:
//
// Paul M. Embree, C Algorithms for Real-Time DSP, 1995
//
// TODO: Neon Intrinsics Optimization

#define BAND_COUNT 7

class Equalizer {

 private:
  static float  leftHist[BAND_COUNT][2];
  static float rightHist[BAND_COUNT][2];
  static float      gain[BAND_COUNT];

  static const float bpf[BAND_COUNT][5];  // Bandpass Filter Coefficients

  float  iirFilter(float input, const float *coef, int n, float *history);
  void adjustGain(char c);

  // Optimized version (when n == 1 and coefficients 4 and 5 are 0 and -1)
  inline float iirFilter2(float input, const float *coef, float *history)
  {
    const float *coefPtr;
    float *hist1Ptr, *hist2Ptr;
    float output, newHist, history1, history2;
  
    coefPtr = coef;

    hist1Ptr = history;
    hist2Ptr = hist1Ptr + 1;

    output = input * *coefPtr++;

    history1 = *hist1Ptr;
    history2 = *hist2Ptr;
    
    output -= history1 * *coefPtr++;
    newHist = output - history2 * *coefPtr;

    output  = newHist;
    output -= history2;

    *hist2Ptr = *hist1Ptr;
    *hist1Ptr = newHist;

    return output;
  }

 public:
   Equalizer();
  ~Equalizer();

  void process(buffp buff, int frameCount);
  void interactiveAdjust();
};

#endif
