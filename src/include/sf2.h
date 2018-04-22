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
  const char * unit;
} generators[generatorDescriptorCount] = {
  { "startAddrsOffset",           1, false,      0,   0,  true, "smpls"      }, //  0
  { "endAddrsOffset",             1, false,      0,   0,  true, "smpls"      }, //  1
  { "startloopAddrsOffset",       1, false,      0,   0,  true, "smpls"      }, //  2
  { "endloopAddrsOffset",         1, false,      0,   0,  true, "smpls"      }, //  3
  { "startAddrsCoarseOffset",     1, false,      0,   0,  true, "32k smpls"  }, //  4
  { "modLfoToPitch",              1, false,      0,   0, false, "cent fs"    }, //  5
  { "vibLfoToPitch",              1, false,      0,   0, false, "cent fs"    }, //  6
  { "modEnvToPitch",              1, false,      0,   0, false, "cent fs"    }, //  7
  { "initialFilterFc",            1,  true,  13500,   0, false, "cent"       }, //  8
  { "initialFilterQ",             1, false,      0,   0, false, "cB"         }, //  9
  { "modLfoToFilterFc",           1, false,      0,   0, false, "cent fs"    }, // 10
  { "modEnvToFilterFc",           1, false,      0,   0, false, "cent fs"    }, // 11
  { "endAddrsCoarseOffset",       1, false,      0,   0,  true, "32k smpls"  }, // 12
  { "modLfoToVolume",             1, false,      0,   0, false, "cB fs"      }, // 13
  { "unused1",                   -1, false,      0,   0, false, ""           }, // 14
  { "chorusEffectsSend",          1, false,      0,   0, false, "0.1%"       }, // 15
  { "reverbEffectsSend",          1, false,      0,   0, false, "0.1%"       }, // 16
  { "pan",                        1,  true,      0,   0, false, "0.1%"       }, // 17
  { "unused2",                   -1, false,      0,   0, false, ""           }, // 18
  { "unused3",                   -1, false,      0,   0, false, ""           }, // 19
  { "unused4",                   -1, false,      0,   0, false, ""           }, // 20
  { "delayModLFO",                1,  true, -12000,   0, false, "timecent"   }, // 21
  { "freqModLFO",                 1,  true,      0,   0, false, "cent"       }, // 22
  { "delayVibLFO",                1,  true, -12000,   0, false, "timecent"   }, // 23
  { "freqVibLFO",                 1,  true,      0,   0, false, "cent"       }, // 24
  { "delayModEnv",                1,  true, -12000,   0, false, "timecent"   }, // 25
  { "attackModEnv",               1,  true, -12000,   0, false, "timecent"   }, // 26
  { "holdModEnv",                 1,  true, -12000,   0, false, "timecent"   }, // 27
  { "decayModEnv",                1,  true, -12000,   0, false, "timecent"   }, // 28
  { "sustainModEnv",              1,  true,      0,   0, false, "-0.1%"      }, // 29
  { "releaseModEnv",              1,  true, -12000,   0, false, "timecent"   }, // 30
  { "keynumToModEnvHold",         1, false,      0,   0, false, "tcent/key"  }, // 31
  { "keynumToModEnvDecay",        1, false,      0,   0, false, "tcent/key"  }, // 32
  { "delayVolEnv",                1, false,      0,   0, false, "timecent"   }, // 33
  { "attackVolEnv",               1, false,      0,   0, false, "timecent"   }, // 34
  { "holdVolEnv",                 1, false,      0,   0, false, "timecent"   }, // 35
  { "decayVolEnv",                1, false,      0,   0, false, "timecent"   }, // 36
  { "sustainVolEnv",              1, false,      0,   0, false, "cB attn"    }, // 37
  { "releaseVolEnv",              1, false,      0,   0, false, "timecent"   }, // 38
  { "keynumToVolEnvHold",         1, false,      0,   0, false, "tcent/key"  }, // 39
  { "keynumToVolEnvDecay",        1, false,      0,   0, false, "tcent/key"  }, // 40
  { "instrument",                 1, false,      0,   0, false, "index"      }, // 41
  { "reserved1",                 -1, false,      0,   0, false, ""           }, // 42
  { "keyRange",                   0,  true,      0, 127, false, "MIDI ky#"   }, // 43
  { "velRange",                   0,  true,      0, 127, false, "MIDI vel"   }, // 44
  { "startloopAddrsCoarseOffset", 1, false,      0,   0,  true, "smpls"      }, // 45
  { "keynum",                     1, false,      0,   0,  true, "MIDI ky#"   }, // 46
  { "velocity",                   1, false,      0,   0,  true, "MIDI vel"   }, // 47
  { "initialAttenuation",         1, false,      0,   0, false, "cB"         }, // 48
  { "reserved2",                 -1, false,      0,   0, false, ""           }, // 49
  { "endloopAddrsCoarseOffset",   1, false,      0,   0,  true, "smpls"      }, // 50
  { "coarseTune",                 1, false,      0,   0, false, "semitone"   }, // 51
  { "fineTune",                   1, false,      0,   0, false, "cent"       }, // 52
  { "sampleID",                   1, false,      0,   0, false, "index"      }, // 53
  { "sampleModes",                1,  true,      0,   0,  true, "Bit Flags"  }, // 54
  { "reserved3",                 -1, false,      0,   0, false, ""           }, // 55
  { "scaleTuning",                1,  true,    100,   0, false, "cent/key"   }, // 56
  { "exclusiveClass",             1, false,      0,   0,  true, "arbitrary#" }, // 57
  { "overridingRootKey",          1, false,      0,   0,  true, "MIDI ky#"   }, // 58
  { "unused5",                   -1, false,      0,   0, false, ""           }, // 59
  { "endOper",                   -1, false,      0,   0, false, ""           }  // 60
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

struct SFModulator {
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
  SFModulator sfModSrcOper;
  SFGenerator sfModDestOper : 16;
  int16_t modAmount;
  SFModulator sfModAmtSrcOper;
  SFTransform sfModTransOper : 16;
};

struct rangesType {
  uint8_t byLo;
  uint8_t byHi;
};

union  genAmountType {
  rangesType ranges;
  uint16_t   shAmount;
  uint16_t   wAmount;
};

struct sfGenList {
  SFGenerator sfGenOper : 16;
  genAmountType genAmount;
};

#endif