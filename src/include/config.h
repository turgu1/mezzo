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

#ifndef _CONFIG_
#define _CONFIG_

#ifdef CONFIG
  #define PUBLIC
#else
  #define PUBLIC extern
#endif

#include <boost/program_options.hpp>
#include <string>

namespace po = boost::program_options;

class Configuration
{
private:
  static po::options_description gen;
  static po::options_description conf;
  static po::options_description hidden;
  static po::options_description visible;

  po::options_description cmdlineOptions;

  void usage();
  void showCopyright();

public:
  po::variables_map configMap;
  std::string configFile;
  std::string soundFontFilename;

  std::string sf2Folder;
  std::string inputSf2;

  uint32_t samplingRate;
  bool     replayEnabled;

  bool interactive;
  bool silent;
  
  bool metEnabled;
  int  metBeatsPerSecond;
  int  metBeatsPerMeasure;

  float equalizer_v60;
  float equalizer_v150;
  float equalizer_v400;
  float equalizer_v1000;
  float equalizer_v2400;
  float equalizer_v6000;
  float equalizer_v15000;

  std::string midiDeviceName;
  int         midiChannel;
  int         midiSustainTreshold;
  int         midiTranspose;

  float reverbRoomSize;
  float reverbDamping;
  float reverbWidth;
  float reverbDryWet;
  float reverbApGain;

  std::string pcmDeviceName;

  uint16_t    volume;
  float       masterVolume;

  bool loadConfig(int argc, char **argv);
  bool showConfig();
};

PUBLIC Configuration config;

#undef PUBLIC
#endif
