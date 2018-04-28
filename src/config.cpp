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

po::options_description Configuration::gen("Generic options");
po::options_description Configuration::conf("Configuration options");
po::options_description Configuration::hidden("Hidden options");
po::options_description Configuration::visible("Allowed options");


void Configuration::usage()
{
  std::cout << std::endl << MEZZO_VERSION << std::endl << std::endl;
  std::cout << "Usage: mezzo [options] [sound font v2 library filename]" << std::endl << std::endl;
  std::cout << visible << std::endl;
  std::cout << std::endl << "Copyright (c) 2018, Guy Turcotte" << std::endl;
}

void Configuration::showCopyright()
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

bool Configuration::loadConfig(int argc, char **argv)
{
  try {
    char filename[256];

    gen.add_options()
      ("copyright,C",   "show copyright information")
      ("help,h",        "produce this help message")
      ("version,v",     "show version then exit")
      ("config-file,c", po::value<std::string>(&configFile)->default_value(".mezzo.conf"),
                        "name of configuration file")
    ;

    conf.add_options()
      ("volume",                  po::value<uint16_t>(&volume),
                                  "Master Volume")
      ("sampling-rate",           po::value<uint32_t>(&samplingRate)->default_value(44100),
                                  "sampling rate")
      ("interactive,i",           "run with interactive mode")
      ("silent,s",                "don't show any message on console")
      ("replay,r",                po::value<bool>(&replayEnabled)->default_value(false),
                                  "keep last part of song for replay")
      ("sf2-folder,d",            po::value<std::string>(&sf2Folder),
                                  "set folder to find sf2 libraries")
      ("pcm-device-nbr",          po::value<int>(&pcmDeviceNbr)->default_value(-1),
                                  "PCM Output Device Nbr")
      ("pcm-device-name",         po::value<std::string>(&pcmDeviceName),
                                  "PCM Output Device Name")
      ("midi-channel",            po::value<int>(&midiChannel),
                                  "Midi Channel or -1 for omni")
      ("midi-device-nbr",         po::value<int>(&midiDeviceNbr)->default_value(-1),
                                  "Midi Input Device Nbr")
      ("midi-device-name",        po::value<std::string>(&midiDeviceName),
                                  "Midi Input Device Name")
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
      ("equalizer-15000",         po::value<float>(&equalizer_v15000),
                                  "Equalizer  15 kHz")
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
      .run(), configMap);
    po::notify(configMap);

    if (configMap.count("help")) {
      usage();
      return false;
    }

    if (configMap.count("copyright")) {
      showCopyright();
      return false;
    }

    if (configMap.count("version")) {
      std::cout << MEZZO_VERSION << std::endl;
      return false;
    }

    interactive = configMap.count("interactive") != 0;
    silent      = configMap.count("silent") != 0;

    if (strchr(configFile.c_str(), '/')) {
      strncpy(filename, configFile.c_str(), 255);
    }
    else {
      strncpy(filename, getenv("HOME"), 255);
      strncat(filename, "/", 255);
      strncat(filename, configFile.c_str(), 255);
    }

    if (Utils::fileExists(filename)) {
      std::ifstream ifs(filename);
      if (!ifs) {
        logger.ERROR("Cannot open config file: %s", filename);
        return false;
      }
      else {
        po::store(po::parse_config_file(ifs, cmdlineOptions), configMap);
        po::notify(configMap);
      }
    }
    else {
      logger.ERROR("File does not exists: %s", filename);
      return false;
    }

    if (!configMap.count("input-sf2")) {
      logger.ERROR("SoundFont file name required\n");
      return false;
    }
    else {
      filename[0] = '\0';
      if ((strchr(inputSf2.c_str(), '/') == NULL) &&
          configMap.count("sf2-folder")) {
        strncpy(filename, sf2Folder.c_str(), 255);
        strncat(filename, "/", 255);
      }
      strncat(filename, inputSf2.c_str(), 255);
      if (Utils::fileExists(filename)) {
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
