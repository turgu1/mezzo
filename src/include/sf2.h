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

#ifndef _SF2_
#define _SF2_

#pragma pack(push,1)

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
  int8_t       valueType;
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
  { "chorusEffectsSend",          1, false,      0,   0, false,  true,      0,   1000, "0.1%"       }, // 15
  { "reverbEffectsSend",          1, false,      0,   0, false,  true,      0,   1000, "0.1%"       }, // 16
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
  { "keynumToModEnvHold",         1, false,      0,   0, false,  true,  -1200,   1200, "tcent/key"  }, // 31
  { "keynumToModEnvDecay",        1, false,      0,   0, false,  true,  -1200,   1200, "tcent/key"  }, // 32
  { "delayVolEnv",                1, false,      0,   0, false,  true, -12000,   5000, "timecent"   }, // 33
  { "attackVolEnv",               1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 34
  { "holdVolEnv",                 1, false,      0,   0, false,  true, -12000,   5000, "timecent"   }, // 35
  { "decayVolEnv",                1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 36
  { "sustainVolEnv",              1, false,      0,   0, false,  true,      0,   1440, "cB attn"    }, // 37
  { "releaseVolEnv",              1, false,      0,   0, false,  true, -12000,   8000, "timecent"   }, // 38
  { "keynumToVolEnvHold",         1, false,      0,   0, false,  true,  -1200,   1200, "tcent/key"  }, // 39
  { "keynumToVolEnvDecay",        1, false,      0,   0, false,  true,  -1200,   1200, "tcent/key"  }, // 40
  { "instrumentID",               2, false,      0,   0, false, false,      0,      0, "index"      }, // 41
  { "reserved1",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 42
  { "keyRange",                   0,  true,      0, 127, false,  true,      0,    127, "MIDI ky#"   }, // 43
  { "velRange",                   0,  true,      0, 127, false,  true,      0,    127, "MIDI vel"   }, // 44
  { "startloopAddrsCoarseOffset", 2, false,      0,   0,  true, false,      0,      0, "smpls"      }, // 45
  { "keynum",                     1, false,      0,   0,  true, false,      0,      0, "MIDI ky#"   }, // 46
  { "velocity",                   1, false,      0,   0,  true, false,      0,      0, "MIDI vel"   }, // 47
  { "initialAttenuation",         1, false,      0,   0, false,  true,      0,   1440, "cB"         }, // 48
  { "reserved2",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 49
  { "endloopAddrsCoarseOffset",   1, false,      0,   0,  true, false,      0,      0, "smpls"      }, // 50
  { "coarseTune",                 1, false,      0,   0, false,  true,   -120,    120, "semitone"   }, // 51
  { "fineTune",                   1, false,      0,   0, false,  true,    -99,     99, "cent"       }, // 52
  { "sampleID",                   2, false,      0,   0,  true, false,      0,      0, "index"      }, // 53
  { "sampleModes",                2,  true,      0,   0,  true, false,      0,      0, "Bit Flags"  }, // 54
  { "reserved3",                 -1, false,      0,   0, false, false,      0,      0, ""           }, // 55
  { "scaleTuning",                1,  true,    100,   0, false,  true,      0,   1200, "cent/key"   }, // 56
  { "exclusiveClass",             2, false,      0,   0,  true, false,      0,      0, "arbitrary#" }, // 57
  { "overridingRootKey",          1, false,      0,   0,  true,  true,      0,    127, "MIDI ky#"   }, // 58
  { "unused5",                   -1, false,      0,   0, false, false,      0,      0, ""           }, // 59
  { "endOper",                   -1, false,      0,   0, false, false,      0,      0, ""           }  // 60
};

enum SFGenerator {
  sfGenOper_startAddrsOffset = 0,
  sfGenOper_endAddrsOffset,
  sfGenOper_startloopAddrsOffset,
  sfGenOper_endloopAddrsOffset,
  sfGenOper_startAddrsCoarseOffset,
  sfGenOper_modLfoToPitch,
  sfGenOper_vibLfoToPitch,
  sfGenOper_modEnvToPitch,
  sfGenOper_initialFilterFc,
  sfGenOper_initialFilterQ,
  sfGenOper_modLfoToFilterFc,
  sfGenOper_modEnvToFilterFc,
  sfGenOper_endAddrsCoarseOffset,
  sfGenOper_modLfoToVolume,
  sfGenOper_unused1,
  sfGenOper_chorusEffectsSend,
  sfGenOper_reverbEffectsSend,
  sfGenOper_pan,
  sfGenOper_unused2,
  sfGenOper_unused3,
  sfGenOper_unused4,
  sfGenOper_delayModLFO,
  sfGenOper_freqModLFO,
  sfGenOper_delayVibLFO,
  sfGenOper_freqVibLFO,
  sfGenOper_delayModEnv,
  sfGenOper_attackModEnv,
  sfGenOper_holdModEnv,
  sfGenOper_decayModEnv,
  sfGenOper_sustainModEnv,
  sfGenOper_releaseModEnv,
  sfGenOper_keynumToModEnvHold,
  sfGenOper_keynumToModEnvDecay,
  sfGenOper_delayVolEnv,
  sfGenOper_attackVolEnv,
  sfGenOper_holdVolEnv,
  sfGenOper_decayVolEnv,
  sfGenOper_sustainVolEnv,
  sfGenOper_releaseVolEnv,
  sfGenOper_keynumToVolEnvHold,
  sfGenOper_keynumToVolEnvDecay,
  sfGenOper_instrumentID,
  sfGenOper_reserved1,
  sfGenOper_keyRange,
  sfGenOper_velRange,
  sfGenOper_startloopAddrsCoarseOffset,
  sfGenOper_keynum,
  sfGenOper_velocity,
  sfGenOper_initialAttenuation,
  sfGenOper_reserved2,
  sfGenOper_endloopAddrsCoarseOffset,
  sfGenOper_coarseTune,
  sfGenOper_fineTune,
  sfGenOper_sampleID,
  sfGenOper_sampleModes,
  sfGenOper_reserved3,
  sfGenOper_scaleTuning,
  sfGenOper_exclusiveClass,
  sfGenOper_overridingRootKey,
  sfGenOper_unused5,
  sfGenOper_endOper
};

enum SFTransform {
  linear        = 0,
  absoluteValue = 2
};

enum SFSampleLink {
  monoSample      =      1,
  rightSample     =      2,
  leftSample      =      4,
  linkedSample    =      8,
  RomMonoSample   = 0x8001,
  RomRightSample  = 0x8002,
  RomLeftSample   = 0x8004,
  RomLinkedSample = 0x8008
};

struct sfSample {
  char     achSampleName[20];
  uint32_t dwStart;
  uint32_t dwEnd;
  uint32_t dwStartloop;
  uint32_t dwEndloop;
  uint32_t dwSampleRate;
  uint8_t  byOriginalPitch;
  int8_t   chPitchCorrection;
  uint16_t wSampleLink;
  SFSampleLink sfSampleType : 16;
};

struct sfModulator {
  unsigned int index                        : 7;
  bool         midiContinuousControllerFlag : 1;
  unsigned int direction                    : 1;
  unsigned int polarity                     : 1;
  unsigned int type                         : 6;
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
  SFGenerator sfModDestOper   : 16;
  int16_t modAmount;
  sfModulator sfModAmtSrcOper;
  SFTransform sfModTransOper  : 16;
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

#pragma pack(pop)

#endif
