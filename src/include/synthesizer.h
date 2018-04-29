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

  enum setGensType { set, adjust };
  void setGens(sfGenList * gens, uint8_t genCount, setType type);
public:
  inline void setGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, set);
  }
  inline void addGens(sfGenList * gens, uint8_t genCount) {
    setGens(gens, genCount, adjust);
  }
  void setDefaults(Sample * sample);

  void process(buffp buff);
};

#endif
