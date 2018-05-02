#include "mezzo.h"

#include <math.h>
#include <iomanip>

#define centibelAttenuation(x) powf(10.0f, -x / 200.0f)
#define cents(x) powf(2.0f, x / 200.0f)

void Synthesizer::setGens(sfGenList * gens, uint8_t genCount, setGensType type)
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
        loop = startLoop != endLoop;
        break;
      case sfGenOper_endloopAddrsOffset:
        endLoop += gens->genAmount.shAmount;
        loop = startLoop != endLoop;
        break;
      case sfGenOper_startAddrsCoarseOffset:
        start += (32768 * gens->genAmount.shAmount);
        break;
      case sfGenOper_endAddrsCoarseOffset:
        end += (32768 * gens->genAmount.shAmount);
        break;
      case  sfGenOper_startloopAddrsCoarseOffset:
        startLoop += (32768 * gens->genAmount.shAmount);
        break;
      case  sfGenOper_endloopAddrsCoarseOffset:
        endLoop += (32768 * gens->genAmount.shAmount);
        break;
      case sfGenOper_pan:
        pan = (type == set) ?
          gens->genAmount.shAmount :
          (pan + gens->genAmount.shAmount);
        break;
      case  sfGenOper_overridingRootKey:
        rootKey = gens->genAmount.shAmount;
        break;
      case  sfGenOper_initialAttenuation:
        if (type == set) {
          attenuationFactor = centibelAttenuation(gens->genAmount.shAmount);
        }
        else {
          attenuationFactor = attenuationFactor * centibelAttenuation((gens->genAmount.shAmount));
        }
        break;
      case  sfGenOper_velocity:
        velocity = gens->genAmount.wAmount;
        break;
      case  sfGenOper_modLfoToPitch:
      case  sfGenOper_vibLfoToPitch:
      case  sfGenOper_modEnvToPitch:
      case  sfGenOper_initialFilterFc:
      case  sfGenOper_initialFilterQ:
      case  sfGenOper_modLfoToFilterFc:
      case  sfGenOper_modEnvToFilterFc:
      case  sfGenOper_modLfoToVolume:
      case  sfGenOper_unused1:
      case  sfGenOper_chorusEffectsSend:
      case  sfGenOper_reverbEffectsSend:
      case  sfGenOper_unused2:
      case  sfGenOper_unused3:
      case  sfGenOper_unused4:
      case  sfGenOper_delayModLFO:
      case  sfGenOper_freqModLFO:
      case  sfGenOper_delayVibLFO:
      case  sfGenOper_freqVibLFO:
      case  sfGenOper_delayModEnv:
      case  sfGenOper_attackModEnv:
      case  sfGenOper_holdModEnv:
      case  sfGenOper_decayModEnv:
      case  sfGenOper_sustainModEnv:
      case  sfGenOper_releaseModEnv:
      case  sfGenOper_keynumToModEnvHold:
      case  sfGenOper_keynumToModEnvDecay:
      case  sfGenOper_delayVolEnv:
      case  sfGenOper_attackVolEnv:
      case  sfGenOper_holdVolEnv:
      case  sfGenOper_decayVolEnv:
      case  sfGenOper_sustainVolEnv:
      case  sfGenOper_releaseVolEnv:
      case  sfGenOper_keynumToVolEnvHold:
      case  sfGenOper_keynumToVolEnvDecay:
      case  sfGenOper_instrumentID:
      case  sfGenOper_reserved1:
      case  sfGenOper_keyRange:
      case  sfGenOper_velRange:
      case  sfGenOper_keynum:
      case  sfGenOper_reserved2:
      case  sfGenOper_coarseTune:
      case  sfGenOper_fineTune:
      case  sfGenOper_sampleID:
      case  sfGenOper_sampleModes:
      case  sfGenOper_reserved3:
      case  sfGenOper_scaleTuning:
      case  sfGenOper_exclusiveClass:
      case  sfGenOper_unused5:
      case  sfGenOper_endOper:
        break;
    }

    gens++;
  }
}

void Synthesizer::setDefaults(Sample * sample)
{
  start             = sample->getStart();
  end               = sample->getEnd();
  startLoop         = sample->getStartLoop();
  endLoop           = sample->getEndLoop();
  sampleRate        = sample->getSampleRate();
  rootKey           = sample->getPitch();
  correctionFactor  = cents(sample->getCorrection());
  loop              = startLoop != endLoop;
  pan               =    0;
  attenuationFactor = 1.0f;
  velocity          =   -1;
}

void Synthesizer::completeParams()
{
  loop = startLoop != endLoop;
  if (loop) {
    sizeSample = endLoop;
  }
  else {
    sizeSample = end - start;
  }
  sizeLoop  = endLoop - startLoop;
}

void Synthesizer::process(buffp buff)
{
  (void) buff;
}

void Synthesizer::showParams()
{
  using namespace std;

  cout << "Synth:"
       << " root:"        << +rootKey
       << " start:"       << start
       << " end:"         << end
       << " startLoop:"   << startLoop
       << " endLoop:"     << endLoop
       << " pan:"         << pan
       << " sizeSample:"  << sizeSample
       << " sizeLoop:"    << sizeLoop
       << " attenuation:" << fixed << setw(7) << setprecision(5) << attenuationFactor
       << " correction:"  << fixed << setw(7) << setprecision(5) << correctionFactor
       << " velocity:"    << +velocity
       << endl;
}
