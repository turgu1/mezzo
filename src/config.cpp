#include "copyright.h"

#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <string>
#include <exception>

#define CONFIG 1
#include "config.h"

#include "mezzo.h"
#include "utils.h"

static po::options_description gen("Generic options");
static po::options_description conf("Configuration options");
static po::options_description hidden("Hidden options");
static po::options_description cmdlineOptions;
static po::options_description visible("Allowed options");

static void usage()
{
  std::cout << std::endl << MEZZO_VERSION << std::endl << std::endl;
  std::cout << "Usage: mezzo [options] [sound font v2 library filename]" << std::endl << std::endl;
  std::cout << visible << std::endl;
  std::cout << std::endl << "Copyright (c) 2018, Guy Turcotte" << std::endl;
}

static void showCopyright()
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

bool loadConfig(int argc, char **argv)
{
  try {
    char filename[256];

    gen.add_options()
      ("copyright,C",   "show copyright information")
      ("help,h",        "produce this help message")
      ("version,v",     "show version then exit")
      ("config,c",      po::value<std::string>(&configFile)->default_value(".mezzo.conf"),
                        "name of configuration file")
    ;

    conf.add_options()
      ("interactive,i",           "run with interactive mode")
      ("silent,s",                "don't show any message on console")
      ("replay,r",                po::value<bool>(&replayEnabled)->default_value(false),
                                  "keep last part of song for replay")
      ("sf2-folder,d",            po::value<std::string>(&sf2Folder),
                                  "set folder to find sf2 libraries")
      ("equalizer-60",            po::value<float>(&equalizer_v60),
                                  "Equalizer  60 Hz")
      ("equalizer-150",           po::value<float>(&equalizer_v150),
                                   "Equalizer 150 Hz")
      ("equalizer-400",           po::value<float>(&equalizer_v400),
                                  "Equalizer 400 Hz")
      ("equalizer-1000",          po::value<float>(&equalizer_v1000),
                                  "Equalizer   1 kHz")
      ("equalizer-2400",          po::value<float>(&equalizer_v2400),
                                  "Equalizer 2.4 kHz")
      ("equalizer-6000",          po::value<float>(&equalizer_v6000),
                                  "Equalizer   6 kHz")
      ("equalizer-15000",         po::value<float>(&equalizer_v150000),
                                  "Equalizer  15 kHz")
      ("midi-channel",            po::value<int>(&midiChannel),
                                  "Midi Channel or -1 for omni")
      ("midi-device-nbr",         po::value<int>(&midiDeviceNbr),
                                  "Midi Device Nbr")
      ("midi-device-name",        po::value<std::string>(&midiDeviceName),
                                  "Midi Device Name")
      ("midi-sustain-treshold",   po::value<int>(&midiSustainTreshold),
                                  "Midi Sustain Treshold")
      ("midi-transpose",          po::value<int>(&midiTranspose),
                                  "Midi Transpose")
      ("reverb-room-size",        po::value<float>(&reverbRoomSize),
                                  "Reverb Room Size")
      ("reverb-damping",          po::value<float>(&reverbDamping),
                                  "Reverb Damping")
      ("reverb-width",            po::value<float>(&reverbWidth),
                                  "Reverb Width")
      ("reverb-dry-wet",          po::value<float>(&reverbDryWet),
                                  "Reverb Dry / Wet")
      ("reverb-ap-gain",          po::value<float>(&reverbApGain),
                                  "Reverb Ap Gain")
      ("master-volume",           po::value<float>(&masterVolume),
                                  "Master Volume")
      ("pcm-device-nbr",         po::value<int>(&pcmDeviceNbr),
                                  "PCM Device Nbr")
      ("pcm-device-name",        po::value<std::string>(&pcmDeviceName),
                                  "PCM Device Name")
    ;

    hidden.add_options()
      ("input-sf2", po::value<std::string>(&inputSf2), "input sf2 library ")
    ;

    visible.add(gen).add(conf);

    cmdlineOptions.add(gen).add(conf).add(hidden);

    po::positional_options_description p;
      p.add("input-sf2", -1);

    po::store(
      po::command_line_parser(argc, argv)
      .options(cmdlineOptions)
      .positional(p)
      .run(), config);
    po::notify(config);

    if (config.count("help")) {
      usage();
      return false;
    }

    if (config.count("copyright")) {
      showCopyright();
      return false;
    }

    if (config.count("version")) {
      std::cout << MEZZO_VERSION << std::endl;
      return false;
    }

    if (strchr(configFile.c_str(), '/')) {
      strncpy(filename, configFile.c_str(), 255);
    }
    else {
      strncpy(filename, getenv("HOME"), 255);
      strncat(filename, "/", 255);
      strncat(filename, configFile.c_str(), 255);
    }

    if (fileExists(filename)) {
      std::ifstream ifs(filename);
      if (!ifs) {
        logger.ERROR("Cannot open config file: %s", filename);
        return false;
      }
      else {
        po::store(po::parse_config_file(ifs, cmdlineOptions), config);
        po::notify(config);
      }
    }
    else {
      logger.ERROR("File does not exists: %s", filename);
      return false;
    }

    if (!config.count("input-sf2")) {
      logger.ERROR("SoundFont file name required\n");
      return false;
    }
    else {
      filename[0] = '\0';
      if ((strchr(inputSf2.c_str(), '/') == NULL) &&
          config.count("sf2-folder")) {
        strncpy(filename, sf2Folder.c_str(), 255);
        strncat(filename, "/", 255);
      }
      strncat(filename, inputSf2.c_str(), 255);
      if (fileExists(filename)) {
        soundFontFilename = filename;
      }
      else {
        logger.ERROR("File does not exists: %s", filename);
        return false;
      }
    }
  }
  catch(std::exception& e) {
    logger.ERROR(e.what());
    return false;
  }

  return true;
}
