#include "mezzo.h"

#include <math.h>
#include <iomanip>

#define centibelToRatio(x) powf(10.0f, -((float) x) / 200.0f)
#define centsToRatio(x) powf(2.0f, x / 1200.0f)

void Synthesizer::setGens(sfGenList * gens, uint8_t genCount, setGensType type)
{
  int32_t iVal;
  float fVal;

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
          attenuationFactor = centibelToRatio(gens->genAmount.shAmount);
        }
        else {
          attenuationFactor *= centibelToRatio(gens->genAmount.shAmount);
        }
        break;
      case  sfGenOper_velocity:
        velocity = gens->genAmount.wAmount;
        break;
        
      // ----- Volume envelope -----
      
      case  sfGenOper_delayVolEnv:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        delayVolEnv = (type == set) ? iVal : (delayVolEnv + iVal);
        break;
      case  sfGenOper_attackVolEnv:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        attackVolEnv = (type == set) ? iVal : (attackVolEnv + iVal);
        break;
      case  sfGenOper_holdVolEnv:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        holdVolEnv = (type == set) ? iVal : (holdVolEnv + iVal);
        break;
      case  sfGenOper_decayVolEnv:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        decayVolEnv = (type == set) ? iVal : (decayVolEnv + iVal);
        break;
      case  sfGenOper_sustainVolEnv:
        fVal = (gens->genAmount.shAmount >= 1000) ? 0.0f :
               ((gens->genAmount.shAmount <= 0)   ? 1.0f :
                centibelToRatio(gens->genAmount.shAmount));
        sustainVolEnv = (type == set) ? fVal : (sustainVolEnv * fVal);
        break;
      case  sfGenOper_releaseVolEnv:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        releaseVolEnv = (type == set) ? iVal : (releaseVolEnv + iVal);
        break;
      case  sfGenOper_keynumToVolEnvHold:
        break;
      case  sfGenOper_keynumToVolEnvDecay:
        break;
      case  sfGenOper_modLfoToPitch:
      case  sfGenOper_vibLfoToPitch:
      case  sfGenOper_modEnvToPitch:
      case  sfGenOper_initialFilterFc:
      case  sfGenOper_initialFilterQ:
      case  sfGenOper_modLfoToFilterFc:
      case  sfGenOper_modEnvToFilterFc:
      case  sfGenOper_modLfoToVolume:
      case  sfGenOper_chorusEffectsSend:
      case  sfGenOper_reverbEffectsSend:
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
      case  sfGenOper_keynum:
      case  sfGenOper_coarseTune:
      case  sfGenOper_fineTune:
      case  sfGenOper_sampleModes:
      case  sfGenOper_scaleTuning:
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
  start                   = sample->getStart();
  end                     = sample->getEnd();
  startLoop               = sample->getStartLoop();
  endLoop                 = sample->getEndLoop();
  sampleRate              = sample->getSampleRate();
  rootKey                 = sample->getPitch();
  correctionFactor        = centsToRatio(sample->getCorrection());
  loop                    = startLoop != endLoop;
  delayVolEnv             =
  attackVolEnv            =
  holdVolEnv              =
  decayVolEnv             =
  releaseVolEnv           =     0;
  sustainVolEnv           =  1.0f;
  amplVolEnv              =  1.0f;
  // keynumToVolEnvHold      =
  // keynumToVolEnvDecay     =
  pan                     =     0;
  attenuationFactor       =  1.0f;
  velocity                =    -1;
  keyReleased             = false;
}

void Synthesizer::completeParams()
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

  attackVolEnvStart   = delayVolEnv;
  holdVolEnvStart     = attackVolEnvStart + attackVolEnv;
  decayVolEnvStart    = holdVolEnvStart   + holdVolEnv;
  sustainVolEnvStart  = decayVolEnvStart   + decayVolEnv;

  attackVolEnvRate  = attackVolEnv  == 0 ? attenuationFactor : (attenuationFactor / (float) attackVolEnv);
  decayVolEnvRate   = decayVolEnv   == 0 ? attenuationFactor : ((attenuationFactor - sustainVolEnv) / (float) decayVolEnv);
  releaseVolEnvRate = releaseVolEnv == 0 ? attenuationFactor : (sustainVolEnv / (float) releaseVolEnv);
  
  cout
  << "VolEnv:[D:" << delayVolEnv
  << ",A:" << attackVolEnv  << "@" << attackVolEnvRate << "/" << attackVolEnvStart
  << ",H:" << holdVolEnv    << "/" << holdVolEnvStart
  << ",D:" << decayVolEnv   << "@" << decayVolEnvRate  << "/" << decayVolEnvStart
  << ",S:" << sustainVolEnv << "/" << sustainVolEnvStart
  << ",R:" << releaseVolEnv << "@" << releaseVolEnvRate
  << "]" << endl;
  
  pos = 0;
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
       << " velocity:"    << +velocity << endl
       << "         "
       << "VolEnv:[D:" << delayVolEnv
       << ",A:" << attackVolEnv << "@" << setprecision(9) << attackVolEnvRate
       << ",H:" << holdVolEnv
       << ",D:" << decayVolEnv << "@" << setprecision(9) << decayVolEnvRate
       << ",S:" << sustainVolEnv
       << ",R:" << releaseVolEnv << "@" << setprecision(9) << releaseVolEnvRate
       << "]" << endl;
}

void Synthesizer::toStereo(buffp dst, buffp src, uint16_t len)
{
  const float prop  = M_SQRT2 * 0.5;
  const float angle = ((float) pan) * M_PI;

  const float left  = prop * (cos(angle) - sin(angle));
  const float right = prop * (cos(angle) + sin(angle));

  if (left < 0.001) {
    while (len--) {
      *dst++ = 0.0;
      *dst++ = *src++;
    }
  }
  else if (right < 0.001) {
    while (len--) {
      *dst++ = *src++;
      *dst++ = 0.0;
    }
  }
  else {
    while (len--) {
      *dst++ = *src * left;
      *dst++ = *src++ * right;
    }
  }
}

// TODO: Convert from linear attack/decay/release to something better...

bool Synthesizer::volumeEnvelope(buffp dst, buffp src, uint16_t len)
{
  using namespace std;
  
  bool endOfSound = false;
  uint32_t thePos = pos;

  while (len--) {
    if (keyReleased) {                        // release
      if (thePos < (keyReleasedPos + releaseVolEnv)) {
        amplVolEnv -= releaseVolEnvRate;
        cout << "R";
      }
      else {
        amplVolEnv = 0.0f;
        endOfSound = true;
      }
    }
    else {
      if (thePos < attackVolEnvStart) {       // delay
        amplVolEnv = 0.0;
        cout << "D";
      }
      else if (thePos < holdVolEnvStart) {    // attack
        cout << "A";
        amplVolEnv += attackVolEnvRate;
      }
      else if (thePos < decayVolEnvStart) {   // hold
        // amplVolEnv stay as current
        cout << "H";
      }
      else if (thePos < sustainVolEnvStart) { // decay
        amplVolEnv -= decayVolEnvRate;
        cout << "D";
      }
      else {
        cout << "S";
        amplVolEnv = sustainVolEnv;
      }
    }
    amplVolEnv = MAX(MIN(attenuationFactor, amplVolEnv), 0.0f);
    *dst++ = *src++ * amplVolEnv;
    thePos += 1;
  }

  return endOfSound;
}

bool Synthesizer::transform(buffp dst, buffp src, uint16_t len)
{
  bool endOfSound;

  endOfSound = volumeEnvelope(src, src, len);

  toStereo(dst, src, len);

  pos += len;

  // if (endOfSound) std::cout << "End of Sound" << std::endl;
  // std::cout << "[" << amplVolEnv << "]" << std::endl;
  
  return endOfSound;
}
