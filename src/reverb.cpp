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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <termios.h>
#include <iomanip>

#include "mezzo.h"

const int Reverb::comb_m[REVERB_COMB_COUNT] = {
  1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116
};

const int Reverb::ap_m[REVERB_AP_COUNT] = {
  225, 556, 441, 341
};

Reverb::Reverb()
{
  setNewHandler(outOfMemory);

  for (int i = 0; i < REVERB_COMB_COUNT; i++) {

    int  frameCount = comb_m[i];

    // left combs
    leftCombs[i].buff = new sample_t[frameCount + PADDING];
    std::fill(leftCombs[i].buff, leftCombs[i].buff + frameCount + PADDING,  0.0f);

    leftCombs[i].end  = leftCombs[i].buff + frameCount;
    leftCombs[i].head = leftCombs[i].tail = leftCombs[i].buff;
    leftLast[i] = 0.0f;

    // right combs
    frameCount += 23;

    rightCombs[i].buff = new sample_t[frameCount + PADDING];
    std::fill(rightCombs[i].buff, rightCombs[i].buff + frameCount + PADDING,  0.0f);

    rightCombs[i].end  = rightCombs[i].buff + frameCount;
    rightCombs[i].head = rightCombs[i].tail = rightCombs[i].buff;
    rightLast[i] = 0.0f;
  }

  for (int i = 0; i < REVERB_AP_COUNT; i++) {

    int  frameCount = ap_m[i];

    // left all pass filters
    leftAp[i].buff = new sample_t[frameCount + PADDING];
    std::fill(leftAp[i].buff, leftAp[i].buff + frameCount + PADDING,  0.0f);

    leftAp[i].end  = leftAp[i].buff + frameCount;
    leftAp[i].head = leftAp[i].tail = leftAp[i].buff;

    // right all pass filters
    rightAp[i].buff = new sample_t[frameCount + PADDING];
    std::fill(rightAp[i].buff, rightAp[i].buff + frameCount + PADDING,  0.0f);

    rightAp[i].end  = rightAp[i].buff + frameCount;
    rightAp[i].head = rightAp[i].tail = rightAp[i].buff;
  }

  roomSize = config.reverbRoomSize;
  damping  = config.reverbDamping;
  width    = config.reverbWidth;
  dryWet   = config.reverbDryWet;
  apGain   = config.reverbApGain;
}

//----- outOfMemory() ----

void Reverb::outOfMemory()
{
  logger.FATAL("Reverb: Unable to allocate memory.");
}

#define LOAD_YN_M(combs_ptr)                   \
  GETv(&combs_ptr[0], yn_m_ab.val[0]);         \
  GETv(&combs_ptr[1], yn_m_ab.val[1]);         \
  GETv(&combs_ptr[2], yn_m_cd.val[0]);         \
  GETv(&combs_ptr[3], yn_m_cd.val[1]);

#define TRANSPOSE_FORWARD_YN_M()                           \
  yn_m_ab = vuzpq_f32(yn_m_ab.val[0], yn_m_ab.val[1]);     \
  yn_m_cd = vuzpq_f32(yn_m_cd.val[0], yn_m_cd.val[1]);     \
  yn_m_ac = vuzpq_f32(yn_m_ab.val[0], yn_m_cd.val[0]);     \
  yn_m_bd = vuzpq_f32(yn_m_ab.val[1], yn_m_cd.val[1]);

#define TRANSPOSE_BACKWARD_YN_M()                          \
  yn_m_ab = vuzpq_f32(yn_m_ac.val[0], yn_m_bd.val[0]);     \
  yn_m_cd = vuzpq_f32(yn_m_ac.val[1], yn_m_bd.val[1]);     \
  yn_m_ac = vuzpq_f32(yn_m_ab.val[0], yn_m_cd.val[0]);     \
  yn_m_bd = vuzpq_f32(yn_m_ab.val[1], yn_m_cd.val[1]);

#define STORE_YN_M(combs_ptr)                   \
  PUTv(&combs_ptr[0], yn_m_ac.val[0]);          \
  PUTv(&combs_ptr[1], yn_m_bd.val[0]);          \
  PUTv(&combs_ptr[2], yn_m_ac.val[1]);          \
  PUTv(&combs_ptr[3], yn_m_bd.val[1]);

#define FILTER(pos)                                                 \
  input = vdupq_n_f32((buffIn[0] + buffIn[1]) * 0.015f);            \
  yn_m_##pos = vmlaq_n_f32(input,                                   \
                           vmlsq_n_f32(yn_m_##pos,                  \
                                       vsubq_f32(yn_m_##pos, yn_1), \
                                       damping),                    \
                           roomSize);                               \
  yn_1 = yn_m_##pos;                                                \
  vst1q_f32(tmp, yn_1);                                             \
  *o++ += tmp[0] + tmp[1] + tmp[2] + tmp[3];                        \
  buffIn += 2;

#define FILTERS()         \
  FILTER(ac.val[0]);      \
  FILTER(bd.val[0]);      \
  FILTER(ac.val[1]);      \
  FILTER(bd.val[1]);

Reverb::~Reverb()
{
  for (int i = 0; i < REVERB_COMB_COUNT; i++) {
    delete [] leftCombs[i].buff;
    delete [] rightCombs[i].buff;
  }
  for (int i = 0; i < REVERB_AP_COUNT; i++) {
    delete []  leftAp[i].buff;
    delete [] rightAp[i].buff;
  }
}

void Reverb::process(frameRecord & buff)
{
  float        dry = dryWet;
  float        wet = 1.0f - dryWet;
  float apGainPlus = 1.0f + apGain;

  Duration * duration = new Duration;

#if USE_NEON_INTRINSICS

  static float outl[BUFFER_FRAME_COUNT];
  static float outr[BUFFER_FRAME_COUNT];

  buffp o_l;
  buffp o_r;
  buffp buffIn, b;
  int fr;

  std::fill(outl, outl + BUFFER_FRAME_COUNT,  0.0f);
  std::fill(outr, outr + BUFFER_FRAME_COUNT,  0.0f);

  for (fr = 0, o_l = outl, o_r = outr;
       fr < BUFFER_FRAME_COUNT;
       fr += 4, o_l += 4, o_r += 4) {

    fifop lc = leftCombs;
    fifop rc = rightCombs;

    float32x4x2_t yn_m_ab;
    float32x4x2_t yn_m_cd;
    float32x4x2_t yn_m_ac; // partially transposed combs values
    float32x4x2_t yn_m_bd; // ..

    float32x4_t yn_1;
    float32x4_t input;
    float tmp[4];
    buffp o;

    // ==== LEFT ====

    yn_1 = vld1q_f32(&leftLast[0]);

    buffIn = b = &buff[fr].left;
    o = o_l;

    LOAD_YN_M(lc);
    TRANSPOSE_FORWARD_YN_M();
    FILTERS();
    TRANSPOSE_BACKWARD_YN_M();
    STORE_YN_M(lc);

    vst1q_f32(&leftLast[0], yn_1);

    lc += 4;
    buffIn = b;
    o = o_l;

    yn_1 = vld1q_f32(&leftLast[4]);

    LOAD_YN_M(lc);
    TRANSPOSE_FORWARD_YN_M();
    FILTERS();
    TRANSPOSE_BACKWARD_YN_M();
    STORE_YN_M(lc);

    vst1q_f32(&leftLast[4], yn_1);

    // ==== RIGHT ====

    yn_1 = vld1q_f32(&rightLast[0]);

    buffIn = b;
    o = o_r;

    LOAD_YN_M(rc);
    TRANSPOSE_FORWARD_YN_M();
    FILTERS();
    TRANSPOSE_BACKWARD_YN_M();
    STORE_YN_M(rc);

    vst1q_f32(&rightLast[0], yn_1);

    rc += 4;
    buffIn = b;
    o = o_r;

    yn_1 = vld1q_f32(&rightLast[4]);

    LOAD_YN_M(rc);
    TRANSPOSE_FORWARD_YN_M();
    FILTERS();
    TRANSPOSE_BACKWARD_YN_M();
    STORE_YN_M(rc);

    vst1q_f32(&rightLast[4], yn_1);
  }

  //buffIn = b;

  for (fr = 0, buffIn = &buff[0].left, o_l = outl, o_r = outr;
       fr < (BUFFER_FRAME_COUNT >> 2);
       fr++, buffIn += 8, o_l += 4, o_r += 4) {

    float32x4_t left, right;
    left  = vld1q_f32(o_l);
    right = vld1q_f32(o_r);

    fifop lap, rap;
    int i;

    for (i = 0, lap = leftAp, rap = rightAp;
         i < REVERB_AP_COUNT;
         i++, lap++, rap++) {

      float32x4_t vn_m, vn;

      // left
      GETv(lap, vn_m);
      // vn = *o_l + (apGain * vn_m);
      vn = vmlaq_n_f32(left, vn_m, apGain);
      PUTv(lap, vn);
      // *o_l = -vn + vn_m * apGainPlus;
      left = vmlaq_n_f32(vnegq_f32(vn), vn_m, apGainPlus);

      // right
      GETv(rap, vn_m);
      // vn = *o_r + (apGain * vn_m);
      vn = vmlaq_n_f32(right, vn_m, apGain);
      PUTv(rap, vn);
      // *o_r = -vn + vn_m * apGainPlus;
      right = vmlaq_n_f32(vnegq_f32(vn), vn_m, apGainPlus);
    }

    float32x4x2_t bb  = vld2q_f32(buffIn);

    bb.val[0] = vmlaq_n_f32(vmulq_n_f32(bb.val[0], dry), left,  wet);
    bb.val[1] = vmlaq_n_f32(vmulq_n_f32(bb.val[1], dry), right, wet);

    vst2q_f32(buffIn, bb);
  }

#else

  for (int fr = 0; fr < BUFFER_FRAME_COUNT; fr++) {
    int i;
    fifop lc, rc;
    fifop lap, rap;

    float input = (buff[fr].left + buff[fr].right).toFloat() * 0.015f;

    float outl = 0.0f;
    float outr = 0.0f;

    float * llast;
    float * rlast;

    for (i = 0, lc = leftCombs, rc = rightCombs, llast = leftLast, rlast = rightLast;
         i < REVERB_COMB_COUNT;
         i++, lc++, rc++, llast++, rlast++) {

      float yn_m, yn;

      // left
      GET(lc, yn_m);
      yn = input + (roomSize * (yn_m - (damping * (yn_m - *llast))));
      PUT(lc, yn);
      *llast = yn;
      outl += yn;

      // right
      GET(rc, yn_m);
      yn = input + (roomSize * (yn_m - (damping * (yn_m - *rlast))));
      PUT(rc, yn);
      *rlast = yn;
      outr += yn;
    }

    for (i = 0, lap = leftAp, rap = rightAp;
         i < REVERB_AP_COUNT;
         i++, lap++, rap++) {

      float vn_m, vn;

      // left
      GET(lap, vn_m);
      vn = outl + (apGain * vn_m);
      PUT(lap, vn);
      outl = -vn + vn_m * apGainPlus;

      // right
      GET(rap, vn_m);
      vn = outr + (apGain * vn_m);
      PUT(rap, vn);
      outr = -vn + vn_m * apGainPlus;
    }

    buff[fr].left = (buff[fr].left * dry) + (outl * wet);  // left
    buff[fr].right = (buff[fr].right * dry) + (outr * wet);  // right
  }
#endif

  long dur = duration->getElapse();
  reverbMinDuration = reverbMinDuration == 0 ? dur : MIN(reverbMinDuration, dur);
  reverbMaxDuration = MAX(reverbMaxDuration, dur);

  delete duration;
}

//---- adjustValue() ----

void Reverb::adjustValue(char ch)
{
  switch (ch) {
  case 'q':
  case 'a':
    apGain += ch == 'q' ? 0.05f : -0.05f;
    apGain = MIN(MAX(apGain, 0.0f), 1.0f);
    break;
  case 'w':
  case 's':
    roomSize += ch == 'w' ? 0.05f : -0.05f;
    roomSize = MIN(MAX(roomSize, 0.0f), 1.0f);
    break;
  case 'e':
  case 'd':
    damping += ch == 'e' ? 0.05f : -0.05f;
    damping = MIN(MAX(damping, 0.0f), 1.0f);
    break;
  case 'r':
  case 'f':
    width += ch == 'r' ? 0.05f : -0.05f;
    width = MIN(MAX(width, 0.0f), 1.0f);
    break;
  case 't':
  case 'g':
    dryWet += ch == 't' ? 0.05f : -0.05f;
    dryWet = MIN(MAX(dryWet, 0.0f), 1.0f);
    break;
  }
}

//---- interactiveAdjust() ----

void Reverb::interactiveAdjust()
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
  cout << "REVERB ADJUSTMENTS."               << endl;
  cout << "Values must be between 0 and 1.0"  << endl;
  cout << "Please use the following keys:"    << endl << endl;
  cout << "For 0.05 increment steps: qwert"   << endl;
  cout << "For 0.05 decrement steps: asdfg"   << endl;
  cout << "                 To exit: x"       << endl;
  cout << endl;
  cout << "[     Gain RoomSize  Damping    Width   DryWet ]" << endl;

  while (keepRunning) {
    cout << "[";
    cout << setw(9) << apGain;
    cout << setw(9) << roomSize;
    cout << setw(9) << damping;
    cout << setw(9) << width;
    cout << setw(9) << dryWet;
    cout << " ]    \r";

    char ch = getchar();
    if (ch == 'x') break;

    adjustValue(ch);
  }

  cout << endl << endl;

  // Restore buffered echoed character input for stdin
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}
