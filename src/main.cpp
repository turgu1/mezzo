#include "copyright.h"
#include "mezzo.h"

#include <iostream>
#include <csignal>
#include <fenv.h>
#include <pthread.h>

#include "interactive_mode.h"

//---- sigfpe_handler ----

/// The sigfpe_handler() function receives control when a floating point exception occurs.

void sigfpeHandler(int dummy)
{
  (void) dummy;

  std::cerr << "Floating Point Error!" << std::endl;

  exit(1);
}

//---- sigint_handler ----

/// The sigintHanfler() function receives control when a signal is sent to the application. The
/// current behaviour is to start a rundown of Mezzo, setting the keepRunning boolean value
/// to false.

void sigintHandler(int dummy)
{
  (void) dummy;
  keepRunning = false;
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
  pthread_t vFeeder;

  if (pthread_create(&smplFeeder, NULL, samplesFeeder, NULL)) {
    logger.FATAL("Unable to start samplesFeeder thread.");
  }

  if (pthread_create(&vFeeder, NULL, voicesFeeder, NULL)) {
    logger.FATAL("Unable to start voicesFeeder thread.");
  }

  if (config.interactive) {
    InteractiveMode im;
    im.menu();
    keepRunning = false;
  }

  // Here we wait until the two threads have been stopped

  pthread_join(smplFeeder, NULL);
  pthread_join(vFeeder, NULL);

  // Leave gracefully

  delete mezzo;

  logger.INFO("Completed.");

  return SUCCESS;
}
