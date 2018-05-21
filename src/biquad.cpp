#include "mezzo.h"

#include "biquad.h"

bool BiQuad::allActive = true;

void BiQuad::setup()
{
  if ((initialQ == 0.0f) || (initialFc < 1500) || (initialFc > 13500)) {
    active = false;
  }
  else {
    float omega, sinus, cosinus, alpha, a0;
 
    omega   = 2 * M_PI * (centsToFreq(initialFc) / config.samplingRate);
    sinus   = sin(omega);
    cosinus = cos(omega);
    alpha   = sinus / (2 * centibelToRatio(initialQ)); 

    a0 = 1 + alpha;
 
    a1 = (-2 * cosinus) / a0;
    a2 = ( 1 -   alpha) / a0;
    b0 = ( 1 - cosinus) / (2 * a0);
    b1 = ( 1 - cosinus) / a0;

    // b2 = ( 1 - cosinus) / (2 * a0);  <-- Similar to b0
  }

  x1 = x2 = y1 = y2 = 0.0f;

  gain = 1.0f / sqrt(initialQ);
}

void BiQuad::filter(buffp src, uint16_t length) 
{
  if (allActive && active) {
    while (length--) {

      // y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2]
      //                     - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]

      float val = (b0 * *src) + (b1 * x1) + (b0 * x2) - (a1 * y1) - (a2 * y2);

      x2 = x1; x1 = *src;
      y2 = y1; y1 =  val;

      *src++ = val * gain;
    }
  }
}

void BiQuad::showStatus(int spaces)
{
  using namespace std;

  cout << setw(spaces) << ' '
       << "BiQuad: "    << ((allActive && active) ? "Active" : "Inactive")
       << " [Fc:"       << initialFc 
       << ", Q:"        << initialQ
       << ", a1/a0:"    << a1
       << ", a2/a0:"    << a2 
       << ", b0_2/a0:"  << b0 
       << ", b1/a0:"    << b1 
       << ", gain:"     << gain
       << "]"
       << endl;
}