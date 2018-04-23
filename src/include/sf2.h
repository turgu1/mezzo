#ifndef _SF2_
#define _SF2_

#pragma pack(1)

/// Required structs for Sound Font file traversal

struct chunk {
  char     id[4];
  uint32_t len;
  uint8_t  data[1];
};

struct chunkList {
  char     id[4];
  uint32_t len;
  char     listName[4];
  chunk    chunks[1];
};

struct mainChunkList {
  char       id[4];
  uint32_t   len;
  char       listName[4];
  chunkList  chunkLists[1];
};

const int generatorDescriptorCount = 61;
const struct generatorDescriptor {
  const char * name;
  int          valueType;
  bool         hasDefaultValue;
  int16_t      defaultValue;
  int16_t      defaultHighKeyValue;
  bool         instrumentOnly;
  bool         checkRange;
  int16_t      minRange;
  int16_t      highRange;
  const char * unit;
} generatorsDesc[generatorDescriptorCount] = {
  { "startAddrsOffset",           2, false,      0,   0,  true, false,      0,      0, "smpls"      }, //  0
  { "endAddrsOffset",             1, false,      0,   0,  true, false,      0,      0, "smpls"      }, //  1
  { "startloopAddrsOffset",       1, false,      0,   0,  true, false,      0,      0, "smpls"      }, //  2
  { "endloopAddrsOffset",         1, false,      0,   0,  true, false,      0,      0, "smpls"      }, //  3
  { "startAddrsCoarseOffset",     2, false,      0,   0,  true, false,      0,      0, "32k smpls"  }, //  4
  { "modLfoToPitch",              1, false,      0,   0, false,  true, -12000,  12000, "cent fs"    }, //  5
  { "vibLfoToPitch",              1, false,      0,   0, false,  true, -12000,  12000, "cent fs"    }, //  6
  { "modEnvToPitch",              1, false,      0,   0, false,  true, -12000,  12000, "cent fs"    }, //  7
  { "initialFilterFc",            2,  true,  13500,   0, false,  true,   1500,  13500, "cent"       }, //  8
  { "initialFilterQ",             2, false,      0,   0, false,  true,      0,    960, "cB"         }, //  9
  { "modLfoToFilterFc",           1, false,      0,   0, false,  true, -12000,  12000, "cent fs"    }, // 10
  { "modEnvToFilterFc",           1, false,      0,   0, false,  true, -12000,  12000, "cent fs"    }, // 11
  { "endAddrsCoarseOffset",       2, false,      0,   0,  true, false,      0,      0, "32k smpls"  }, // 12
  { "modLfoToVolume",             1, false,      0,   0, false,  true,   -960,    960, "cB fs"      }, // 13
  { "unused1",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 14
  { "chorusEffectsSend",          1, false,      0,   0, false, false,      0,      0, "0.1%"       }, // 15
  { "reverbEffectsSend",          1, false,      0,   0, false, false,      0,      0, "0.1%"       }, // 16
  { "pan",                        1,  true,      0,   0, false,  true,   -500,    500, "0.1%"       }, // 17
  { "unused2",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 18
  { "unused3",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 19
  { "unused4",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 20
  { "delayModLFO",                1,  true, -12000,   0, false,  true, -12000,   5000, "timecent"   }, // 21
  { "freqModLFO",                 1,  true,      0,   0, false,  true, -16000,   4500, "cent"       }, // 22
  { "delayVibLFO",                1,  true, -12000,   0, false,  true, -12000,   5000, "timecent"   }, // 23
  { "freqVibLFO",                 1,  true,      0,   0, false,  true, -16000,   4500, "cent"       }, // 24
  { "delayModEnv",                1,  true, -12000,   0, false,  true, -12000,   5000, "timecent"   }, // 25
  { "attackModEnv",               1,  true, -12000,   0, false,  true, -12000,   8000, "timecent"   }, // 26
  { "holdModEnv",                 1,  true, -12000,   0, false,  true, -12000,   5000, "timecent"   }, // 27
  { "decayModEnv",                1,  true, -12000,   0, false,  true, -12000,   8000, "timecent"   }, // 28
  { "sustainModEnv",              1,  true,      0,   0, false,  true,      0,   1000, "-0.1%"      }, // 29
  { "releaseModEnv",              1,  true, -12000,   0, false,  true, -12000,   8000, "timecent"   }, // 30
  { "keynumToModEnvHold",         1, false,      0,   0, false, false,      0,      0, "tcent/key"  }, // 31
  { "keynumToModEnvDecay",        1, false,      0,   0, false, false,      0,      0, "tcent/key"  }, // 32
  { "delayVolEnv",                1, false,      0,   0, false,  true, -12000,   5000, "timecent"   }, // 33
  { "attackVolEnv",               1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 34
  { "holdVolEnv",                 1, false,      0,   0, false,  true, -12000,   5000, "timecent"   }, // 35
  { "decayVolEnv",                1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 36
  { "sustainVolEnv",              1, false,      0,   0, false,  true,      0,   1440, "cB attn"    }, // 37
  { "releaseVolEnv",              1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 38
  { "keynumToVolEnvHold",         1, false,      0,   0, false, false,      0,      0, "tcent/key"  }, // 39
  { "keynumToVolEnvDecay",        1, false,      0,   0, false, false,      0,      0, "tcent/key"  }, // 40
  { "instrument",                 2, false,      0,   0, false, false,      0,      0, "index"      }, // 41
  { "reserved1",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 42
  { "keyRange",                   0,  true,      0, 127, false,  true,      0,    127, "MIDI ky#"   }, // 43
  { "velRange",                   0,  true,      0, 127, false,  true,      0,    127, "MIDI vel"   }, // 44
  { "startloopAddrsCoarseOffset", 2, false,      0,   0,  true, false,      0,      0, "smpls"      }, // 45
  { "keynum",                     1, false,      0,   0,  true, false,      0,      0, "MIDI ky#"   }, // 46
  { "velocity",                   1, false,      0,   0,  true, false,      0,      0, "MIDI vel"   }, // 47
  { "initialAttenuation",         1, false,      0,   0, false, false,      0,      0, "cB"         }, // 48
  { "reserved2",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 49
  { "endloopAddrsCoarseOffset",   1, false,      0,   0,  true, false,      0,      0, "smpls"      }, // 50
  { "coarseTune",                 1, false,      0,   0, false,  true,   -120,    120, "semitone"   }, // 51
  { "fineTune",                   1, false,      0,   0, false,  true,    -99,     99, "cent"       }, // 52
  { "sampleID",                   2, false,      0,   0, false, false,      0,      0, "index"      }, // 53
  { "sampleModes",                2,  true,      0,   0,  true, false,      0,      0, "Bit Flags"  }, // 54
  { "reserved3",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 55
  { "scaleTuning",                1,  true,    100,   0, false, false,      0,      0, "cent/key"   }, // 56
  { "exclusiveClass",             2, false,      0,   0,  true, false,      0,      0, "arbitrary#" }, // 57
  { "overridingRootKey",          1, false,      0,   0,  true,  true,     -1,    127, "MIDI ky#"   }, // 58
  { "unused5",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 59
  { "endOper",                   -1, false,      0,   0, false, false,      0,      0, ""           }  // 60
};

enum SFGenerator {
  startAddrsOffset = 0,
  endAddrsOffset,
  startloopAddrsOffset,
  endloopAddrsOffset,
  startAddrsCoarseOffset,
  modLfoToPitch,
  vibLfoToPitch,
  modEnvToPitch,
  initialFilterFc,
  initialFilterQ,
  modLfoToFilterFc,
  modEnvToFilterFc,
  endAddrsCoarseOffset,
  modLfoToVolume,
  unused1,
  chorusEffectsSend,
  reverbEffectsSend,
  pan,
  unused2,
  unused3,
  unused4,
  delayModLFO,
  freqModLFO,
  delayVibLFO,
  freqVibLFO,
  delayModEnv,
  attackModEnv,
  holdModEnv,
  decayModEnv,
  sustainModEnv,
  releaseModEnv,
  keynumToModEnvHold,
  keynumToModEnvDecay,
  delayVolEnv,
  attackVolEnv,
  holdVolEnv,
  decayVolEnv,
  sustainVolEnv,
  releaseVolEnv,
  keynumToVolEnvHold,
  keynumToVolEnvDecay,
  instrument,
  reserved1,
  keyRange,
  velRange,
  startloopAddrsCoarseOffset,
  keynum,
  velocity,
  initialAttenuation,
  reserved2,
  endloopAddrsCoarseOffset,
  coarseTune,
  fineTune,
  sampleID,
  sampleModes,
  reserved3,
  scaleTuning,
  exclusiveClass,
  overridingRootKey,
  unused5,
  endOper
};

enum SFTransform {
  linear = 0,
  absoluteValue = 2
};

struct sfModulator {
  unsigned int index : 7;
  bool         midiContinuousControllerFlag : 1;
  unsigned int direction : 1;
  unsigned int polarity : 1;
  unsigned int type : 6;
};

struct sfInst {
  char     achInstName[20];
  uint16_t wInstBagNdx;
};

struct sfPresetHeader {
  char     achPresetName[20];
  uint16_t wPreset;
  uint16_t wBank;
  uint16_t wPresetBagNdx;
  uint32_t dwLibrary;
  uint32_t dwGenre;
  uint32_t dwMorphology;
};

struct sfBag {
  uint16_t wGenNdx;
  uint16_t wModNdx;
};

struct sfModList {
  sfModulator sfModSrcOper;
  SFGenerator sfModDestOper : 16;
  int16_t modAmount;
  sfModulator sfModAmtSrcOper;
  SFTransform sfModTransOper : 16;
};

struct rangesType {
  uint8_t byLo;
  uint8_t byHi;
};

union  genAmountType {
  rangesType ranges;
  int16_t   shAmount;
  uint16_t   wAmount;
};

struct sfGenList {
  SFGenerator sfGenOper : 16;
  genAmountType genAmount;
};

#endif
