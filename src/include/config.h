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
  
  float equalizer_v60;
  float equalizer_v150;
  float equalizer_v400;
  float equalizer_v1000;
  float equalizer_v2400;
  float equalizer_v6000;
  float equalizer_v15000;

  std::string midiDeviceName;
  int         midiChannel;
  int         midiDeviceNbr;
  int         midiSustainTreshold;
  int         midiTranspose;

  float reverbRoomSize;
  float reverbDamping;
  float reverbWidth;
  float reverbDryWet;
  float reverbApGain;

  std::string pcmDeviceName;
  int         pcmDeviceNbr;

  uint16_t    volume;
  float       masterVolume;

  bool loadConfig(int argc, char **argv);
  bool showConfig();
};

PUBLIC Configuration config;

#undef PUBLIC
#endif
