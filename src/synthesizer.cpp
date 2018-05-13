#include "mezzo.h"

#include <math.h>
#include <iomanip>

#define SetOrAdd(x,y) \
  if (type == set) x.set##y(gens->genAmount.shAmount); \
  else x.addTo##y(gens->genAmount.shAmount); break

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
      case  sfGenOper_velocity:
        velocity = gens->genAmount.wAmount;
        break;

      // ----- Volume envelope -----

      case  sfGenOper_delayVolEnv:         SetOrAdd(volEnvelope, Delay        );
      case  sfGenOper_attackVolEnv:        SetOrAdd(volEnvelope, Attack       );
      case  sfGenOper_holdVolEnv:          SetOrAdd(volEnvelope, Hold         );
      case  sfGenOper_decayVolEnv:         SetOrAdd(volEnvelope, Decay        );
      case  sfGenOper_releaseVolEnv:       SetOrAdd(volEnvelope, Release      );
      case  sfGenOper_sustainVolEnv:       SetOrAdd(volEnvelope, Sustain      );
      case  sfGenOper_initialAttenuation:  SetOrAdd(volEnvelope, Attenuation  );
      case  sfGenOper_keynumToVolEnvHold:  SetOrAdd(volEnvelope, KeynumToHold );
      case  sfGenOper_keynumToVolEnvDecay: SetOrAdd(volEnvelope, KeynumToDecay);

      // Low-Pass BiQuad Filter
      case  sfGenOper_initialFilterFc:  SetOrAdd(biQuad, InitialFc);
      case  sfGenOper_initialFilterQ:   SetOrAdd(biQuad, InitialQ );

      // Vibrato

      case  sfGenOper_vibLfoToPitch:    SetOrAdd(vib, Pitch    );
      case  sfGenOper_delayVibLFO:      SetOrAdd(vib, Delay    );
      case  sfGenOper_freqVibLFO:       SetOrAdd(vib, Frequency);

      case  sfGenOper_modLfoToPitch:
      case  sfGenOper_modLfoToFilterFc:
      case  sfGenOper_modLfoToVolume:
      case  sfGenOper_delayModLFO:
      case  sfGenOper_freqModLFO:

      case  sfGenOper_modEnvToPitch:
      case  sfGenOper_modEnvToFilterFc:
      case  sfGenOper_delayModEnv:
      case  sfGenOper_attackModEnv:
      case  sfGenOper_holdModEnv:
      case  sfGenOper_decayModEnv:
      case  sfGenOper_sustainModEnv:
      case  sfGenOper_releaseModEnv:
      case  sfGenOper_keynumToModEnvHold:
      case  sfGenOper_keynumToModEnvDecay:

      case  sfGenOper_chorusEffectsSend:
      case  sfGenOper_reverbEffectsSend:

      case  sfGenOper_keynum:

      case  sfGenOper_coarseTune:
      case  sfGenOper_fineTune:
      case  sfGenOper_scaleTuning:

      case  sfGenOper_sampleModes:
      case  sfGenOper_exclusiveClass:

      case  sfGenOper_keyRange:
      case  sfGenOper_velRange:
      case  sfGenOper_instrumentID:
      case  sfGenOper_sampleID:
      case  sfGenOper_reserved1:
      case  sfGenOper_reserved2:
      case  sfGenOper_reserved3:
      case  sfGenOper_unused1:
      case  sfGenOper_unused2:
      case  sfGenOper_unused3:
      case  sfGenOper_unused4:
      case  sfGenOper_unused5:
      case  sfGenOper_endOper:
        break;
    }

    gens++;
  }
}

void Synthesizer::setDefaults(Sample * sample)
{
  start            = sample->getStart();
  end              = sample->getEnd();
  startLoop        = sample->getStartLoop();
  endLoop          = sample->getEndLoop();
  sampleRate       = sample->getSampleRate();
  rootKey          = sample->getPitch();
  correctionFactor = centsToRatio(sample->getCorrection());
  loop             = startLoop != endLoop;

  pan              =     0;
  velocity         =    -1;
}

void Synthesizer::completeParams(uint8_t note)
{
  using namespace std;

  loop = startLoop != endLoop;
  if (loop) {
    sizeSample = endLoop;
  }
  else {
    sizeSample = end - start;
  }
  sizeLoop  = endLoop - startLoop;

  volEnvelope.setup();
  vib.setup(note);
  biQuad.setup();

  //std::cout << "completeParams" << std::endl;

  //showParams();

  pos = 0;
}

void Synthesizer::process(buffp buff)
{
  (void) buff;
}

void Synthesizer::showStatus(int spaces)
{
  using namespace std;

  cout << setw(spaces) << ' '
       << "Synth:" 
       << "[root:"        << +rootKey
       << " velocity:"    << +velocity 
       << " start:"       << start
       << " end:"         << end
       << " startLoop:"   << startLoop
       << " endLoop:"     << endLoop
       << " pan:"         << pan
       << " sizeSample:"  << sizeSample
       << " sizeLoop:"    << sizeLoop
       << " correction:"  << fixed << setw(7) << setprecision(5) << correctionFactor
       << "]" << endl;

  volEnvelope.showStatus(spaces + 4);
          vib.showStatus(spaces + 4);
       biQuad.showStatus(spaces + 4);
}

bool Synthesizer::transform(buffp dst, buffp src, uint16_t length)
{
  bool endOfSound;

  biQuad.filter(src, length);

  endOfSound = volEnvelope.transform(src, length, pos);

  toStereo(dst, src, length);

  pos += length;

  // if (endOfSound) std::cout << "End of Sound" << std::endl;
  // std::cout << "[" << amplVolEnv << "]" << std::endl;

  return endOfSound;
}

