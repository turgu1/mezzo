#include "copyright.h"

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
