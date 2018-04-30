#ifndef _SYNTHESIZER_
#define _SYNTHESIZER_

// A synthesizer is containing the generators and code to
// transform samples in relashionship with the generators. A synthesizer
// is attached to each voice and is central to the transformation of
// sounds in the way the soundfont designer wanted it to be ear.

class Synthesizer {

private:
  int16_t      pan;
  uint32_t     start;
  uint32_t     end;
  uint32_t     startLoop;
  uint32_t     endLoop;
  uint32_t     sampleRate;
  uint32_t     sizeSample;
  uint32_t     sizeLoop;
  float        attenuationFactor;
  uint8_t      rootKey;
  bool         loop;

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
  inline uint8_t  getRootKey()     { return rootKey; }
};

#endif
