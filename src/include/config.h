#include "copyright.h"

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

PUBLIC std::string configFile;
PUBLIC std::string sf2Folder;
PUBLIC std::string inputSf2;
PUBLIC std::string soundFontFilename;

PUBLIC po::variables_map config;

PUBLIC bool replayEnabled;

PUBLIC float equalizer_v60;
PUBLIC float equalizer_v150;
PUBLIC float equalizer_v400;
PUBLIC float equalizer_v1000;
PUBLIC float equalizer_v2400;
PUBLIC float equalizer_v6000;
PUBLIC float equalizer_v15000;

PUBLIC std::string midiDeviceName;
PUBLIC int         midiChannel;
PUBLIC int         midiDeviceNbr;
PUBLIC int         midiSustainTreshold;
PUBLIC int         midiTranspose;

PUBLIC float reverbRoomSize;
PUBLIC float reverbDamping;
PUBLIC float reverbWidth;
PUBLIC float reverbDryWet;
PUBLIC float reverbApGain;






PUBLIC std::string pcmDeviceName;
PUBLIC int         pcmDeviceNbr;

PUBLIC float      masterVolume;

bool loadConfig(int argc, char **argv);

#undef PUBLIC
#endif
