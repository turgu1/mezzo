#include "copyright.h"

#include "PIano.h"

#include "fifo.h"

//---- Fifo() ----

Fifo::Fifo()
{
  setNewHandler(outOfMemory);

  head = tail = 0;
  count = 0;
  stateLock = 0;

  for (int i = 0; i < FIFO_BUFFER_COUNT; i++) {
    buff[i] = new sample_t[BUFFER_SAMPLE_COUNT];
    frameCount[i] = 0;
  }
}

//---- ~Fifo() ----

Fifo::~Fifo()
{
  for (int i = 0; i < FIFO_BUFFER_COUNT; i++) {
    delete [] buff[i];
  }
}

//----- outOfMemory() ----

void Fifo::outOfMemory()
{
  logger.FATAL("Fifo: Unable to allocate memory.");
}

//---- showState() ----

void Fifo::showState()
{
  using namespace std;

  cout << "FIFO STATE:" << endl << "----------" << endl;

  cout << "Head: "  << head  << endl;
  cout << "Tail: "  << tail  << endl;
  cout << "Count: " << count << endl;

  cout << "[End]" << endl;
}
