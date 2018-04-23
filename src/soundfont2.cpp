#include "mezzo.h"
#include "soundfont2.h"

SoundFont2::SoundFont2(std::string & sf2Filename)
{
  loaded = false;

  assert(sizeof(sfModulator   ) ==  2);
  assert(sizeof(sfInst        ) == 22);
  assert(sizeof(sfPresetHeader) == 38);
  assert(sizeof(sfBag         ) ==  4);
  assert(sizeof(sfModList     ) == 10);
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
      loaded = retrieveInstrumentList() && retrievePresetList();
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
    delete *i;
  }

  std::vector<Preset *>::iterator p;
  for (p = presets.begin(); p != presets.end(); p++) {
    delete *p;
  }

  if (file.is_open()) file.close();
}

bool SoundFont2::loadInstrument(std::string & instrumentName)
{
  (void) instrumentName;

  return true;
}

bool SoundFont2::loadInstrument(int instrumentIndex)
{
  (void) instrumentIndex;

  return true;
}

bool SoundFont2::loadPreset(std::string & presetName)
{
  (void) presetName;

  return true;
}

bool SoundFont2::loadPreset(int presetIndex)
{
  (void) presetIndex;

  return true;
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

  // ---- instrument headers ----

  ck = findChunk("inst", *ckl);
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
                     inst[1].wInstBagNdx - inst[0].wInstBagNdx,
                     ibags,
                     igens,
                     imods));
  }

  assert(strcmp(inst->achInstName, "EOI") == 0);
  return true;
}

bool SoundFont2::retrievePresetList()
{
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

  // ---- preset headers ----

  ck = findChunk("phdr", *ckl);
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

    presets.push_back(new Preset(name, preset->wPreset, preset->wBank));
  }

  assert(strcmp(preset->achPresetName, "EOP") == 0);
  return true;
}
