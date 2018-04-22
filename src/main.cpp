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

/// Show the Copyright Notice to the end user.

void showCopyright()
{
  std::cout <<
"\n\
 Simplified BSD License\n\
 ----------------------\n\
\n\
 Copyright (c) 2018, Guy Turcotte\n\
 All rights reserved.\n\
 \n\
 Redistribution and use in source and binary forms, with or without\n\
 modification, are permitted provided that the following conditions are met:\n\
 \n\
 1. Redistributions of source code must retain the above copyright notice, this\n\
    list of conditions and the following disclaimer.\n\
 2. Redistributions in binary form must reproduce the above copyright notice,\n\
    this list of conditions and the following disclaimer in the documentation\n\
    and/or other materials provided with the distribution.\n\
 \n\
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n\
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n\
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR\n\
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n\
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n\
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n\
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n\
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
 \n\
 The views and conclusions contained in the software and documentation are those\n\
 of the authors and should not be interpreted as representing official policies,\n\
 either expressed or implied, of the FreeBSD Project.\n\
" << std::endl;
}

void usage()
{
  using namespace std;

  cout << endl;
  cout << "Usage: mezzo [options...] <sf2_file>"     << endl << endl;
  cout << "  -i  Interactive Mode."       << endl;
  cout << "  -s  Silent: no logging msg." << endl;
  cout << "  -c  Show Copyright Notice."  << endl;
  cout << "  -h  This help text."         << endl << endl;
  cout << "(c) 2018, Guy Turcotte"        << endl;
}
//---- MAIN ----

#include <getopt.h>

int main(int argc, char **argv)
{
  char opt;
  std::string * sf2_filename = NULL;
  
  signal(SIGINT, sigintHandler);
  signal(SIGFPE, sigfpeHandler);

  feenableexcept(FE_DIVBYZERO|FE_INVALID);

  interactive = false;
  silent      = false;
  keepRunning = true;
  
  while ((opt = getopt(argc, argv, "ishc")) != (char)-1) {
    switch (opt) {
    case 'i':
      interactive = true;
      break;
    case 's':
      silent = true;
      break;
    case 'c':
      showCopyright();
      exit(0);
      break;
    case 'h':
    case '?':
      usage();
      exit(0);
      break;
    }
  }

  if (optind < argc) {
    sf2_filename = new std::string(argv[optind]);
  }
  else {
    usage();
    exit(1);
  }
  
  mezzo = new Mezzo(*sf2_filename);

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
  
  if (sf2_filename) delete sf2_filename;
  
  delete mezzo;

  logger.INFO("Completed.");
  
  return SUCCESS;
}  
