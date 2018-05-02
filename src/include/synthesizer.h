#ifndef _SYNTHESIZER_
#define _SYNTHESIZER_

// A synthesizer is containing the generators and code to
// transform samples in relashionship with the generators. A synthesizer
// is attached to each voice and is central to the transformation of
// sounds in the way the soundfont designer wanted it to be ear.

class Synthesizer {

private:
  uint32_t pos;
  uint32_t start;
  uint32_t end;
  uint32_t startLoop;
  uint32_t endLoop;
  uint32_t sampleRate;
  uint32_t sizeSample;
  uint32_t sizeLoop;
  uint32_t  delayVolEnv;
  uint32_t  attackVolEnv;
  uint32_t  holdVolEnv;
  uint32_t  decayVolEnv;
  uint32_t  releaseVolEnv;
  uint32_t  attackVolEnvStart;
  uint32_t  holdVolEnvStart;
  uint32_t  decayVolEnvStart;
  uint32_t  sustainVolEnvStart;
  uint32_t  keyReleasedPos;
  float    attackVolEnvRate;
  float    decayVolEnvRate;
  float    releaseVolEnvRate;
  float    sustainVolEnv;
  float    amplVolEnv;
  float    attenuationFactor;
  float    correctionFactor;
  int16_t  pan;
  uint8_t  rootKey;
  int8_t   velocity;
  bool     loop;
  bool     keyReleased;

  enum setGensType { set, adjust };
  void setGens(sfGenList * gens, uint8_t genCount, setGensType type);
public:
  inline void setGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, set);
  }
  inline void addGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, adjust);
  }
  void setDefaults(Sample * sample);

  void process(buffp buff);

  void showParams();
  void completeParams();

  inline uint32_t getStart()       { return start; }
  inline uint32_t getEnd()         { return end; }
  inline uint32_t getStartLoop()   { return startLoop; }
  inline uint32_t getEndLoop()     { return endLoop; }
  inline uint32_t getSampleRate()  { return sampleRate; }
  inline bool     isLooping()      { return loop; }
  inline uint16_t getPan()         { return pan; }
  inline uint32_t getSizeSample()  { return sizeSample; }
  inline uint32_t getSizeLoop()    { return sizeLoop; }
  inline float    getAttenuation() { return attenuationFactor; }
  inline float    getCorrection()  { return correctionFactor; }
  inline uint8_t  getRootKey()     { return rootKey; }
  inline int8_t   getVelocity()    { return velocity; }

  void keyHasBeenReleased() {
    keyReleased = true;
    keyReleasedPos = pos;
    releaseVolEnvRate = releaseVolEnv == 0 ? 1.0 : (amplVolEnv / (float)releaseVolEnv);
  }

  bool volumeEnvelope(buffp dst, buffp src, uint16_t len);
  void  toStereo(buffp dst, buffp src, uint16_t len);
  bool transform(buffp dst, buffp src, uint16_t len);
};

#endif
