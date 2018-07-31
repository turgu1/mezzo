#include "copyright.h"
#include "mezzo.h"

#include <iostream>
#include <csignal>
#include <fenv.h>
#include <pthread.h>

#include "interactive_mode.h"

//---- sigfpeHandler ----

/// The sigfpeHandler() function receives control when a floating point exception occurs.

void sigfpeHandler(int dummy)
{
  (void) dummy;

  std::cerr << "Floating Point Error!" << std::endl;

  exit(1);
}

//---- sigintHandler ----

/// The sigintHanfler() function receives control when a signal is sent to the application. The
/// current behaviour is to start a rundown of Mezzo, setting the keepRunning boolean value
/// to false and broadcasting to threads.

void sigintHandler(int dummy)
{
  (void) dummy;
  stopThreads();
}

//---- MAIN ----

int main(int argc, char **argv)
{
  signal(SIGINT, sigintHandler);
  signal(SIGFPE, sigfpeHandler);

  #if __LINUX__
    feenableexcept(FE_DIVBYZERO|FE_INVALID);
  #endif

  keepRunning = true;

  if (!config.loadConfig(argc, argv)) return 1;

  mezzo = new Mezzo();

  assert(mezzo != NULL);

  pthread_t smplFeeder;
  pthread_t vFeeder1;
  pthread_t vFeeder2;

  if (pthread_create(&smplFeeder, NULL, samplesFeeder, NULL)) {
    logger.FATAL("Unable to start samplesFeeder thread.");
  }

  if (pthread_create(&vFeeder1, NULL, voicesFeeder1, NULL)) {
    logger.FATAL("Unable to start voicesFeeder1 thread.");
  }

  if (pthread_create(&vFeeder2, NULL, voicesFeeder2, NULL)) {
    logger.FATAL("Unable to start voicesFeeder2 thread.");
  }

  if (config.interactive) {
    InteractiveMode im;
    im.menu();
    stopThreads();
  }

  // Here we wait until the two threads have been stopped

  pthread_join(smplFeeder, NULL);
  pthread_join(vFeeder1, NULL);
  pthread_join(vFeeder2, NULL);

  // Leave gracefully

  delete mezzo;

  logger.INFO("Completed.");

  return 0;
}
