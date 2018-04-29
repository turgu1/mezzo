#include "mezzo.h"

void Synthesizer::setGens(sfGenList * gens, uint8_t genCount, setType type)
{
  while (genCount--) {
    switch (gens->sfGenOper) {
      case sfGenOper_startAddrsOffset:
        start += gens->genAmount.shAmount;
        break;
      case sfGenOper_endAddrsOffset:
        end += gens->genAmount.shAmount;
        break;
      case sfGenOper_startloopAddrsOffset:
        startLoop += gens->genAmount.shAmount;
        break;
      case sfGenOper_endloopAddrsOffset:
        endLoop += gens->genAmount.shAmount;
        break;
      case sfGenOper_startAddrsCoarseOffset:
        start += (32768 * gens->genAmount.shAmount);
        break;
      case sfGenOper_pan: 
        pan = (type == set) ?
                gens->genAmount.shAmount :
                (pan + gens->genAmount.shAmount);
        break;
    }

    gens++;
  }
}

void Synthesizer::setDefaults(Sample * sample)
{
  start     = sample->getStart();
  end       = sample->getEnd();
  startLoop = sample->getStartLoop();
  endLoop   = sample->getEndLoop();
}

void process(buffp buff)
{}
