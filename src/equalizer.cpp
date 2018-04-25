#include "copyright.h"

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <iomanip>

#include "mezzo.h"

float Equalizer::gain[BAND_COUNT] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

float Equalizer::leftHist[BAND_COUNT][2] = {
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 }
};

float Equalizer::rightHist[BAND_COUNT][2] = {
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 },
  { 0.0, 0.0 }
};

// Bandpass filter coefficients for a 44.1 kHz samping rate
// Center freqs are: 60, 150, 400, 1000, 2400, 6000, 15000

const float Equalizer::bpf[BAND_COUNT][5] = {
  { 0.0025579741, -1.9948111773, 0.9948840737, 0.0, -1.0 },
  { 0.0063700872, -1.9868060350, 0.9872598052, 0.0, -1.0 },
  { 0.0168007612, -1.9632060528, 0.9663984776, 0.0, -1.0 },
  { 0.0408578217, -1.8988473415, 0.9182843566, 0.0, -1.0 },
  { 0.0914007276, -1.7119922638, 0.8171985149, 0.0, -1.0 },
  { 0.1845672876, -1.0703823566, 0.6308654547, 0.0, -1.0 },
  { 0.3760778010,  0.6695288420, 0.2478443682, 0.0, -1.0 }
};

Equalizer::Equalizer()
{
  gain[0] = cfg.equalizer.v60;
  gain[1] = cfg.equalizer.v150;
  gain[2] = cfg.equalizer.v400;
  gain[3] = cfg.equalizer.v1000;
  gain[4] = cfg.equalizer.v2400;
  gain[5] = cfg.equalizer.v6000;
  gain[6] = cfg.equalizer.v15000;
}

Equalizer::~Equalizer()
{
}

float Equalizer::iirFilter(float input, const float *coef, int n, float *history)
{
  const float *coefPtr;
  float *hist1Ptr, *hist2Ptr;
  float output, newHist, history1, history2;

  coefPtr = coef;

  hist1Ptr = history;
  hist2Ptr = hist1Ptr + 1;

  output = input * *coefPtr++;

  for(int i = 0; i < n; i++) {

    history1 = *hist1Ptr;
    history2 = *hist2Ptr;
    
    output -= history1 * *coefPtr++;
    newHist = output - history2 * *coefPtr++;

    output  = newHist + history1 * *coefPtr++;
    output += history2 * *coefPtr++;

    *hist2Ptr++ = *hist1Ptr;
    *hist1Ptr++ = newHist;

    hist1Ptr++;
    hist2Ptr++;
  }

  return output;
}

void Equalizer::process(buffp buff, int frameCount)
{
  for (int i = 0; i < frameCount; i++) {

    // Left
    float sigIn = *buff;
    float sigOut = sigIn;
    for (int j = 0; j < BAND_COUNT; j++) {
      sigOut += gain[j] * iirFilter2(sigIn, bpf[j], leftHist[j]);
    }
    *buff++ = sigOut;

    // Right
    sigIn = *buff;
    sigOut = sigIn;
    for (int j = 0; j < BAND_COUNT; j++) {
      sigOut += gain[j] * iirFilter2(sigIn, bpf[j], rightHist[j]);
    }
    *buff++ = sigOut;
  }
}

//---- adjustGain(char c) ----

void Equalizer::adjustGain(char c)
{
  char up[8]   = "qwertyu";
  char down[8] = "asdfghj";

  char *pos;
  if ((pos = strchr(up, c)) != NULL) {
    gain[pos - up] += 0.05f;
  }
  else if ((pos = strchr(down, c)) != NULL) {
    gain[pos - down] -= 0.05f;
  }
}

//---- interactiveAdjust() ----

void Equalizer::interactiveAdjust()
{
 // Setup unbuffered nonechoed character input for stdin
  struct termios old_tio, new_tio;
  tcgetattr(STDIN_FILENO, &old_tio);
  new_tio = old_tio;
  new_tio.c_lflag &= (~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

  using namespace std;

  cout.precision(2);
  cout.setf(ios::showpoint);

  cout << endl << endl;
  cout << "EQUALIZER ADJUSTMENTS" << endl;
  cout << "Please use the following keys:"    << endl << endl;
  cout << "For 0.05 increment steps: qwertyu" << endl;
  cout << "For 0.05 decrement steps: asdfghj" << endl;
  cout << "                 To exit: x" << endl;
  cout << endl;
  cout << "[     60    150    400   1000   2400   6000  15000 ]" << endl;

  while (keepRunning) {
    cout << "[";
    for (int i = 0; i < BAND_COUNT; i++) {
      cout << setw(7) << equalizer->gain[i];
    }
    cout << " ]    \r";

    char ch = getchar();
    if (ch == 'x') break;

    adjustGain(ch);
  }

  cout << endl << endl;

  // Restore buffered echoed character input for stdin
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}
