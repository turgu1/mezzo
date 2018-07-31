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

#ifndef _FIFO_
#define _FIFO_

#include <sched.h>

#define FIFO_BUFFER_COUNT 6

#define __INC while (__sync_lock_test_and_set(&stateLock, 1)); \
  count += 1;                                                  \
  __sync_lock_release(&stateLock);

#define __DEC while (__sync_lock_test_and_set(&stateLock, 1)); \
  count -= 1;                                                  \
  __sync_lock_release(&stateLock);

#define __CLR while (__sync_lock_test_and_set(&stateLock, 1)); \
  count = 0;                                                   \
  __sync_lock_release(&stateLock);

class Fifo : public NewHandlerSupport<Fifo> {

 private:
  sampleRecord buff[FIFO_BUFFER_COUNT];
  int   sampleCount[FIFO_BUFFER_COUNT];

  volatile int head, tail;
  volatile int count;
  volatile int stateLock;

  static void outOfMemory();

 public:
   Fifo();
  ~Fifo();

  void showState();

  inline sampleRecord & getHead() { return buff[head]; }
  inline sampleRecord & getTail() { return buff[tail]; }

  inline void  pop() { if (++head >= FIFO_BUFFER_COUNT) head = 0; __DEC }
  inline void push() { if (++tail >= FIFO_BUFFER_COUNT) tail = 0; __INC }

  inline bool isFull()  { return count == FIFO_BUFFER_COUNT; }
  inline bool isEmpty() { return count == 0; }

  inline void setSampleCount(int count) { sampleCount[tail] = count; }
  inline int  getSampleCount() { return sampleCount[head]; }

  inline void clear() { head = tail = 0; __CLR }
};

#undef __DEC
#undef __INC
#undef __CLR
#endif
