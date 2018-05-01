#include "copyright.h"

#include <iomanip>

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

  currentPreset   = NULL;
  firstMidiPreset = NULL;

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
  std::vector<Preset *>::iterator p;
  for (p = presets.begin(); p != presets.end(); p++) {
    delete * p;
  }

  std::vector<Instrument *>::iterator i;
  for (i = instruments.begin(); i != instruments.end(); i++) {
    delete * i;
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

bool SoundFont2::loadInstrument(std::string & instrumentName,
                                rangesType  & keys)
{
  for (uint16_t i = 0; i < instruments.size(); i++) {
    if (instrumentName == instruments[i]->getName()) {
      return loadInstrument(i, keys);
    }
  }
  return false;
}

bool SoundFont2::loadInstrument(uint16_t     instrumentIndex,
                                rangesType & keys)
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
    if (presetName == presets[i]->getName()) return loadPreset(presets[i]);
  }
  return false;
}

bool SoundFont2::loadMidiPreset(uint8_t bankNbr, uint8_t midiNbr)
{
  Preset * p = firstMidiPreset;
  while (p && ((p->getBankNbr() != bankNbr) || (p->getMidiNbr() != midiNbr))) {
    p = p->getNextMidiPreset();
  }
  if (p != NULL) return loadPreset(p);
  return false;
}

bool SoundFont2::loadFirstPreset()
{
  return loadPreset(firstMidiPreset);
}

bool SoundFont2::loadNextPreset()
{
  if (currentPreset == NULL) return false;
  return loadPreset(currentPreset->getNextMidiPreset());
}

bool SoundFont2::loadPreviousPreset()
{
  Preset * p = firstMidiPreset;
  Preset * po = NULL;

  if (p == NULL) return false;

  while ((p != NULL) && (p != currentPreset)) {
    po = p;
    p = p->getNextMidiPreset();
  }
  if (po == NULL) return false;
  return loadPreset(po);
}

bool SoundFont2::loadPreset(uint16_t presetIndex)
{
  if (presetIndex < presets.size()) {
    return loadPreset(presets[presetIndex]);
  }
  return false;
}

bool SoundFont2::loadPreset(Preset * preset)
{
  if (preset == NULL) return false;

  if (currentPreset) {
    currentPreset->unload();
    currentPreset = NULL;
  }

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

  currentPreset = preset;
  return currentPreset->load(pbags, pgens, pmods);
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

    Preset *p;
    presets.push_back(
      p = new Preset(name,
                     preset[0].wPreset,
                     preset[0].wBank,
                     preset[0].wPresetBagNdx,
                     preset[1].wPresetBagNdx - preset[0].wPresetBagNdx));
    addPresetToMidiList(p);
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

  int16_t * dta = (int16_t *) ck->data;

  // ---- samples (8 low bits) ----

  #if samples24bits
    int8_t * dta24 = NULL;
    uint32_t dtaLen = ck->len;
    ck = findChunk("sm24", *ckl);
    if ((ck != NULL) && (ck->len >= dtaLen))  {
      assert(memcmp(ck->id, "sm24", 4) == 0);
      dta24 = (int8_t *) ck->data;
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

    // if (smplInfo->dwSampleRate != config.samplingRate) {
    //   logger.WARNING("Sample Rate (%d Hz) not compatible with the application", smplInfo->dwSampleRate);
    // }

    samples.push_back(new Sample(*smplInfo));
  }

  logger.DEBUG("Samples attributes retrieval completed.");

  return true;
}

void SoundFont2::addPresetToMidiList(Preset *preset)
{
  if (firstMidiPreset == NULL) {
    firstMidiPreset = preset;
    preset->setNextMidiPreset(NULL);
  }
  else {
    Preset * p  = firstMidiPreset;
    Preset * po = NULL;
    while (p) {
      if (p->getBankNbr() > preset->getBankNbr()) {
        break;
      }
      else if (p->getBankNbr() == preset->getBankNbr()) {
        if (p->getMidiNbr() > preset->getMidiNbr()) break;
      }
      po = p;
      p = p->getNextMidiPreset();
    }
    preset->setNextMidiPreset(p);
    if (po == NULL) {
      firstMidiPreset = preset;
    }
    else {
      po->setNextMidiPreset(preset);
    }
  }
}

void SoundFont2::showMidiPresetList()
{
  using namespace std;

  Preset * p = firstMidiPreset;
  int i = 0;
  int qty = presets.size();

  for (i = 0; i < 3; i++) cout << "Idx   Bk Mid  Name                 ";
  cout << endl;
  for (i = 0; i < 3; i++) cout << "---   -- ---  -------------------- ";
  cout << endl;

  int nxt = 0;
  while (i < qty) {
    p = firstMidiPreset;
    for (int j = 0; (p != NULL) && (j < nxt); j++) p = p->getNextMidiPreset();
    if (p != NULL) {
      unsigned k = 0;
      while (k < presets.size()) {
        if (presets[k] == p) break;
        k++;
      }
      cout << setw(3) << right << k << ": "
           << "[" << setw(2) << right << p->getBankNbr()
                  << setw(4) << right << p->getMidiNbr() << "] "
           << setw(20) << left << p->getName() << " ";
      if ((++i % 3) == 0) cout << endl;
    }
    else {
      cout << endl;
    }
    nxt = (nxt + ((qty + 2) / 3)) % qty;
  }
  if ((i % 3) != 0) cout << endl;
}
