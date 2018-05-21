#ifndef _BIQUAD_
#define _BIQUAD_

// The algorithm come from the following url:
//
//    http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
//

class BiQuad
{
private:
  bool active;              // This instance
  static bool allActive;  // For all instances

  float initialFc;
  float initialQ;
  float a1, a2, b0, b1;
  float x1, x2, y1, y2;
  float gain;

public:
  BiQuad()
  {
    // initialFc = centsToRatio(13500) / config.samplingRate;
    initialFc = 13500;
    initialQ  =  0.0f;
    active    = false;
    gain      =  1.3f;
  }

  inline void setInitialFc  (int16_t fc) { initialFc  = fc; }
  inline void addToInitialFc(int16_t fc) { initialFc += fc; }
  inline void setInitialQ   (int16_t  q) { initialQ   =  q; }
  inline void addToInitialQ (int16_t  q) { initialQ  +=  q; }

  static bool toggleAllActive() { return allActive = !allActive; }
  static bool areAllActive   () { return allActive;              }

  void setup();
  void filter(buffp src, uint16_t length);
  void showStatus(int spaces);
};

#endif