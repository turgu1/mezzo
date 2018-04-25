#include "copyright.h"
#include "mezzo.h"

#include <iostream>
#include <csignal>
#include <fenv.h>

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
/// current behaviour is to start a rundown of PIano, setting the keepRunning boolean value
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

  interactive = false;
  silent      = false;
  keepRunning = true;

  if (!loadConfig(argc, argv)) return 1;

  interactive = config.count("interactive") != 0;
  silent      = config.count("silent") != 0;
  
  mezzo = new Mezzo();

  // std:thread opener;
  // std:thread feeder;
  //
  // if (piano == NULL) {
  //   logger.FATAL("Unable to allocate memory for PIano.");
  // }
  //
  // if (pthread_create(&feeder, NULL, samplesFeeder, NULL)) {
  //   logger.FATAL("Unable to start samplesFeeder thread.");
  //  }
  //
  // if (pthread_create(&opener, NULL, sampleFileOpener, NULL)) {
  //   logger.FATAL("Unable to start sample_file_opener thread.");
  // }
  //
  // if (interactive) {
  //   InteractiveMode im;
  //   im.menu();
  //   keepRunning = false;
  // }
  //
  // // Here we wait until the two threads have been stopped
  //
  // pthread_join(feeder, NULL);
  // pthread_join(opener, NULL);
  //
  // // Leave gracefully

  delete mezzo;

  logger.INFO("Completed.");

  return SUCCESS;
}
