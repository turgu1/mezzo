#include "mezzo.h"

#include "biquad.h"

bool BiQuad::allActive = true;

void BiQuad::setInitialFc  (int16_t fc) 
{ 
  std::cout << "FilterFc:" << fc << ", " << std::flush;

  initialFc  = (abs((((float) fc) * _1200TH_ROOT_OF_2))) / config.samplingRate; 
  active = true; 
}

void BiQuad::addToInitialFc(int16_t fc) 
{ 
  std::cout << "+FilterFc:" << fc << ", " << std::flush;

  initialFc += (((float) fc) * _1200TH_ROOT_OF_2) / config.samplingRate; 
  active = true;  
}

