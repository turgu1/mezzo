#include "copyright.h"

#ifndef _CONFIG_
#define _CONFIG_

#ifdef CONFIG
# define PUBLIC
#else
# define PUBLIC extern
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

bool loadConfig(int argc, char **argv);

#undef PUBLIC
#endif
