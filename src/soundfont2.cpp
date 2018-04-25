#include "copyright.h"

#include "mezzo.h"
#include "soundfont2.h"

SoundFont2::SoundFont2(std::string & sf2Filename)
{
  setNewHandler(outOfMemory);

  loaded = false;

  assert(sizeof(sfModulator   ) ==  2);
  assert(sizeof(sfInst        ) == 22);
  assert(sizeof(sfPresetHeader) == 38);
  assert(sizeof(sfBag         ) ==  4);
  assert(sizeof(sfModList     ) == 10);
  assert(sizeof(sfSample      ) == 46);
  assert(sizeof(rangesType    ) ==  2);
  assert(sizeof(genAmountType ) ==  2);
  assert(sizeof(sfGenList     ) ==  4);

  file.open(sf2Filename);
  if (!file.is_open()) {
    logger.ERROR("Unable to open file %s.", sf2Filename.c_str());
  }
  else {
    data = file.data();

    mainChunkList & ck = *(mainChunkList *) data;

    if ((memcmp(ck.id,       "RIFF", 4) == 0) &&
        (memcmp(ck.listName, "sfbk", 4) == 0)) {
      loaded = retrieveInstrumentList() && retrievePresetList() && retrieveSamples();
    }
    else {
      logger.ERROR("Unrecognizable SF2 file format: %s", sf2Filename.c_str());
    }
  }
}

SoundFont2::~SoundFont2()
{
  std::vector<Instrument *>::iterator i;
  for (i = instruments.begin(); i != instruments.end(); i++) {
    delete * i;
  }

  std::vector<Preset *>::iterator p;
  for (p = presets.begin(); p != presets.end(); p++) {
    delete * p;
  }

  std::vector<Sample *>::iterator s;
  for (s = samples.begin(); s != samples.end(); s++) {
    delete * s;
  }

  if (file.is_open()) file.close();
}

void SoundFont2::outOfMemory()
{
  logger.FATAL("SoundFont2: Unable to allocate memory.");
}

bool SoundFont2::loadInstrument(std::string & instrumentName, rangesType & keys)
{
  for (uint16_t i = 0; i < instruments.size(); i++) {
    if (instrumentName == instruments[i]->getName()) return loadInstrument(i, keys);
  }
  return false;
}

bool SoundFont2::loadInstrument(uint16_t instrumentIndex, rangesType & keys)
{
  if (instrumentIndex >= instruments.size()) return false;

  assert(instruments[instrumentIndex] != NULL);
  
  chunkList * ckl = findChunkList("pdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "pdta", 4) == 0);

  // ---- ibags ----

  chunk * ck = findChunk("ibag", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "ibag", 4) == 0);

  sfBag * ibags = (sfBag *) ck->data;

  // ---- imods ----

  ck = findChunk("imod", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "imod", 4) == 0);

  sfModList * imods = (sfModList *) ck->data;

  // ---- igens ----

  ck = findChunk("igen", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "igen", 4) == 0);

  sfGenList * igens = (sfGenList *) ck->data;

  return instruments[instrumentIndex]->load(ibags, igens, imods, keys);
}

bool SoundFont2::loadPreset(std::string & presetName)
{
  for (uint16_t i = 0; i < presets.size(); i++) {
    if (presetName == presets[i]->getName()) return loadPreset(i);
  }
  return false;
}

bool SoundFont2::loadPreset(uint16_t presetIndex)
{
  if (presetIndex >= presets.size()) return false;

  assert(presets[presetIndex] != NULL);
  
  chunkList * ckl = findChunkList("pdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "pdta", 4) == 0);

  // ---- pbags ----

  chunk * ck = findChunk("pbag", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "pbag", 4) == 0);

  sfBag * pbags = (sfBag *) ck->data;

  // ---- pmods ----

  ck = findChunk("pmod", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "pmod", 4) == 0);

  sfModList * pmods = (sfModList *) ck->data;

  // ---- pgens ----

  ck = findChunk("pgen", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "pgen", 4) == 0);

  sfGenList * pgens = (sfGenList *) ck->data;

  return presets[presetIndex]->load(pbags, pgens, pmods);
}

chunk * SoundFont2::findChunk(char const * id, chunkList & src)
{
  chunk * ck = src.chunks;
  int len = src.len;

  while (len > 0) {
    // logger.DEBUG("Chunk Name: %4.4s", ck->id);

    if (memcmp(ck->id, id, 4) == 0) return ck;
    len -= ck->len + 8;
    ck = (chunk *)(((uint8_t *) ck) + 8 + ck->len);
  }
  return NULL;
}

chunkList * SoundFont2::findChunkList(char const * name)
{
  mainChunkList & main = *(mainChunkList *) data;
  chunkList * ckl = main.chunkLists;
  int len = main.len - 4;

  while (len > 0) {
    assert(memcmp(ckl->id, "LIST", 4) == 0);

    //logger.DEBUG("Chunk List Name: %4.4s", ckl->listName);

    if (memcmp(ckl->listName, name, 4) == 0) return ckl;
    len -= ckl->len + 8;
    ckl = (chunkList *)(((uint8_t *) ckl) + 8 + ckl->len);
  }
  return NULL;
}

bool SoundFont2::retrieveInstrumentList()
{
  chunkList * ckl = findChunkList("pdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "pdta", 4) == 0);

  // ---- instrument headers ----

  chunk * ck = findChunk("inst", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "inst", 4) == 0);

  sfInst * inst = (sfInst *) ck->data;

  // ---- generate the instruments ----

  for (unsigned int pos = 0;
       pos < (ck->len - sizeof(sfInst));
       pos += sizeof(sfInst), inst++) {
    char name[21];
    strncpy(name, inst->achInstName, 20);
    name[20] = '\0';

    instruments.push_back(
      new Instrument(name,
                     inst[0].wInstBagNdx,
                     inst[1].wInstBagNdx - inst[0].wInstBagNdx));
  }

  assert(strcmp(inst->achInstName, "EOI") == 0);
  return true;
}

bool SoundFont2::retrievePresetList()
{
  chunkList * ckl = findChunkList("pdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "pdta", 4) == 0);

  // ---- preset headers ----

  chunk * ck = findChunk("phdr", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "phdr", 4) == 0);

  sfPresetHeader * preset = (sfPresetHeader *) ck->data;

  // ---- generate the presets

  for (unsigned int pos = 0;
       pos < (ck->len - sizeof(sfPresetHeader));
       pos += sizeof(sfPresetHeader), preset++) {
    char name[21];
    strncpy(name, preset->achPresetName, 20);
    name[20] = '\0';

    presets.push_back(
      new Preset(name,
                 preset[0].wPreset,
                 preset[0].wBank,
                 preset[0].wPresetBagNdx,
                 preset[1].wPresetBagNdx - preset[0].wPresetBagNdx));
  }

  assert(strcmp(preset->achPresetName, "EOP") == 0);
  return true;
}

bool SoundFont2::retrieveSamples()
{
  chunkList * ckl = findChunkList("sdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "sdta", 4) == 0);

  // ---- samples (16 high bits) ----

  chunk * ck = findChunk("smpl", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "smpl", 4) == 0);

  uint16_t * dta = (uint16_t *) ck->data;

  // ---- samples (8 low bits) ----

  #if samples24bits
    uint8_t * dta24 = NULL;
    uint32_t dtaLen = ck->len;
    ck = findChunk("sm24", *ckl);
    if ((ck != NULL) && (ck->len >= dtaLen))  {
      assert(memcmp(ck->id, "sm24", 4) == 0);
      dta24 = (uint8_t *) ck->data;
    }
    Sample::setSamplesLocation(dta, dta24);
  #else
    Sample::setSamplesLocation(dta);
  #endif

  // --

  ckl = findChunkList("pdta");
  if (ckl == NULL) return false;
  assert(memcmp(ckl->listName, "pdta", 4) == 0);

  // ---- samples attributes ----

  ck = findChunk("shdr", *ckl);
  if (ck == NULL) return false;
  assert(memcmp(ck->id, "shdr", 4) == 0);

  sfSample * smplInfo = (sfSample *) ck->data;

  for (unsigned int pos = 0;
       pos < (ck->len - sizeof(sfSample));
       pos += sizeof(sfSample), smplInfo++) {

    if (smplInfo->dwSampleRate != 44100) {
      logger.WARNING("Sample Rate (%d Hz) not compatible with the application", smplInfo->dwSampleRate);
    }

    samples.push_back(new Sample(*smplInfo));
  }

  logger.DEBUG("Samples attributes retrieval completed.");

  return true;
}
