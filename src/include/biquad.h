#ifndef _BIQUAD_
#define _BIQUAD_

// The algorithm come from the following url:
//
//   http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
//

class BiQuad
{
private:
  bool active;              // This instance
  static bool allActive;  // For all instances

  float initialFc;
  float initialQ;
  float a0, a1, b1, b2, z1, z2;
  float gain;

public:
  BiQuad()
  {
    initialFc = centsToRatio(13500) / config.samplingRate;
    initialQ  =  1.0f;
    active = false;
    gain = 1.3f;
  }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive() { return allActive; }

  inline void setInitialFc  (int16_t fc) { initialFc  = centsToRatio(fc) / config.samplingRate; 
                                           active = true; }
  inline void addToInitialFc(int16_t fc) { initialFc *= centsToRatio(fc) / config.samplingRate; 
                                           active = true; }

  inline void setInitialQ  (int16_t q) { initialQ  = ((float) q) / 10.0; }
  inline void addToInitialQ(int16_t q) { initialQ += ((float) q) / 10.0; }

  void setup()
  {
    if (initialQ == 1.0f) {
      a0 = 1.0f;
      a1 = b1 = b2 = 0.0f;
    }
    else {
      float K = tan(M_PI * initialFc);
      float K2 = K * K;
      float norm = 1 / (1 + K / initialQ + K2);

      a0 = K2 * norm;
      a1 = 2 * a0;
      b1 = 2 * (K2 - 1) * norm;
      b2 = (1 - K / initialQ + K2) * norm;
    }
    z1 = z2 = 0.0f;

  }

  inline void filter(buffp src, uint16_t length) 
  {
    if (allActive && active) {
      while (length--) {
        float val = *src * a0 + z1;
        z1 = (*src * a1) + z2 - (b1 * val);
        z2 = (*src * a0) - (b2 * val);
        *src++ = val * gain;
      }
    }
  }

  void showStatus(int spaces)
  {
    using namespace std;

    cout << setw(spaces) << ' '
         << "BiQuad: " << ((allActive && active) ? "Active" : "Inactive")
         << " [Fc:"    << initialFc 
         << ", Q:"     << initialQ
         << ", a0:"    << a0 
         << ", a1:"    << a1 
         << ", b1:"    << b1 
         << ", b2:"    << b2 
         << ", gain:"  << gain
         << "]"
         << endl;
  }
};

#if 0
// This code is incomplete and may be removed at some point
// Butterworth low-pass parameters

// ToDo: Status of Butterworth filtering

void Synthesizer::biQuadSetup()
{
  if (initialFilterQ == 1.0f) {
    a0 = 1.0f;
    a1 = a2 = b1 = b2 = 0.0f;
  }
  else {
    float alpha = 1 / tan(M_PI * initialFilterFc / config.samplingRate);

    a0 = 1 / (1 + (2 * alpha) + (alpha * alpha));
    a1 = 2 * a0;
    a2 = a0;
    b1 = 2 * a0 * (1 - (alpha * alpha));
    b2 = a0 * (1 - (2 * alpha) + (alpha * alpha));
  }
  z1 = z2 = 0.0f;
}
#endif


#endif