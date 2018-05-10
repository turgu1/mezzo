#ifndef _BIQUAD_
#define _BIQUAD_

class BiQuad
{
private:
  static bool enabled;

  float initialFc;
  float initialQ;
  float a0, a1, a2, b1, b2, z1, z2;

public:
  BiQuad()
  {
    initialFc = centsToRatio(13500) / config.samplingRate;
    initialQ  =  1.0f;  // DB
  }

  static inline bool isEnabled() { return enabled; }
  static inline void toggle() { enabled = !enabled; }

  inline void setInitialFc  (int16_t fc) { initialFc  = centsToRatio(fc) / config.samplingRate; }
  inline void addToInitialFc(int16_t fc) { initialFc *= centsToRatio(fc) / config.samplingRate; }

  inline void setInitialQ  (int16_t q) { initialQ  = ((float) q) / 10.0; }
  inline void addToInitialQ(int16_t q) { initialQ += ((float) q) / 10.0; }

  void setup()
  {
    if (initialQ == 1.0f) {
      a0 = 1.0f;
      a1 = a2 = b1 = b2 = 0.0f;
    }
    else {
      float K = tan(M_PI * initialFc);
      float norm = 1 / (1 + K / initialQ + K * K);

      a0 = K * K * norm;
      a1 = 2 * a0;
      a2 = a0;
      b1 = 2 * (K * K - 1) * norm;
      b2 = (1 - K / initialQ + K * K) * norm;
    }
    z1 = z2 = 0.0f;

  }

  inline void filter(buffp src, uint16_t length) 
  {
    if (enabled) {
      while (length--) {
        float val = *src * a0 + z1;
        z1 = (*src * a1) + z2 - (b1 * val);
        z2 = (*src * a2) - (b2 * val);
        *src++ = val;
      }
    }
  }

  void showStatus()
  {
    using namespace std;

    cout << "[Fc:"  << initialFc 
         << ", Q:"  << initialQ
         << ", a0:" << a0 
         << ", a1:" << a1 
         << ", a2:" << a2
         << ", b1:" << b1 
         << ", b2:" << b2 
         << "]"
         << endl << flush;
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