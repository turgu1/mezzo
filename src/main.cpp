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

#include <iostream>
#include <csignal>
#include <fenv.h>
#include <pthread.h>

#include "mezzo.h"

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
  // pthread_t prtMonitor;

  if (pthread_create(&smplFeeder, NULL, samplesFeeder, NULL)) {
    logger.FATAL("Unable to start samplesFeeder thread.");
  }

  if (pthread_create(&vFeeder1, NULL, voicesFeeder1, NULL)) {
    logger.FATAL("Unable to start voicesFeeder1 thread.");
  }

  if (pthread_create(&vFeeder2, NULL, voicesFeeder2, NULL)) {
    logger.FATAL("Unable to start voicesFeeder2 thread.");
  }

  // if (pthread_create(&prtMonitor, NULL, portMonitor, NULL)) {
  //   logger.FATAL("Unable to start portMonitor thread.");
  // }

  if (config.interactive) {
    InteractiveMode im;
    im.menu();
    stopThreads();
  }
  else {
    portMonitor(NULL);
  }

  // Here we wait until the two threads have been stopped

  pthread_join(smplFeeder, NULL);
  pthread_join(vFeeder1,   NULL);
  pthread_join(vFeeder2,   NULL);
  // pthread_join(prtMonitor, NULL);

  // Leave gracefully

  delete mezzo;

  logger.INFO("Completed.");

  return 0;
}
