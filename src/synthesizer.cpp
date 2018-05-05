#include "mezzo.h"

#include <math.h>
#include <iomanip>

bool Synthesizer::filterEnabled = true;

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
          attenuationFactor = centibelToRatio(-gens->genAmount.shAmount);
        }
        else {
          attenuationFactor *= centibelToRatio(-gens->genAmount.shAmount);
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
                centibelToRatio(-gens->genAmount.shAmount));
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

      // Low-Pass BiQuad Filter
      case  sfGenOper_initialFilterFc:
        fVal = centsToRatio(gens->genAmount.shAmount) / config.samplingRate;
        initialFilterFc = (type == set) ? fVal : initialFilterFc * fVal;
        break;
      case  sfGenOper_initialFilterQ:
        fVal = float(gens->genAmount.shAmount) / 10.0;
        initialFilterQ = (type == set) ? fVal : initialFilterQ + fVal;
        break;

      // Vibrato

      case  sfGenOper_vibLfoToPitch:
        iVal = gens->genAmount.shAmount;
        vibLfoToPitch = (type == set) ? iVal : (iVal + vibLfoToPitch);
        break;
      case  sfGenOper_delayVibLFO:
        iVal = gens->genAmount.shAmount == -32768 ? 0 :
               config.samplingRate * centsToRatio(gens->genAmount.shAmount);
        delayVibLFO = (type == set) ? iVal : (delayVibLFO + iVal);
        break;
      case  sfGenOper_freqVibLFO:
        iVal = centsToDuration(gens->genAmount.shAmount);
        durationVibLFO = (type == set) ? iVal : (iVal + durationVibLFO);
        break;

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
  initialFilterFc         = centsToRatio(13500) / config.samplingRate;
  initialFilterQ          =  1.0f;  // DB
  vibLfoToPitch           =     0;
  delayVibLFO             =     0;
  durationVibLFO          =     0;
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

  biQuadSetup();
  std::cout << "completeParams" << std::endl;
  showParams();
  // cout
  // << "VolEnv:[D:" << delayVolEnv
  // << ",A:" << attackVolEnv  << "@" << attackVolEnvRate << "/" << attackVolEnvStart
  // << ",H:" << holdVolEnv    << "/" << holdVolEnvStart
  // << ",D:" << decayVolEnv   << "@" << decayVolEnvRate  << "/" << decayVolEnvStart
  // << ",S:" << sustainVolEnv << "/" << sustainVolEnvStart
  // << ",R:" << releaseVolEnv << "@" << releaseVolEnvRate
  // << "]" << endl;

  pos = 0;
}

void Synthesizer::process(buffp buff)
{
  (void) buff;
}

void Synthesizer::showParams()
{
  using namespace std;

  cout << "Synth@" << this << ": "
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
       << ",A:" << attackVolEnv  << "@" << attackVolEnvRate << "/" << attackVolEnvStart
       << ",H:" << holdVolEnv    << "/" << holdVolEnvStart
       << ",D:" << decayVolEnv   << "@" << decayVolEnvRate  << "/" << decayVolEnvStart
       << ",S:" << sustainVolEnv << "/" << sustainVolEnvStart
       << ",R:" << releaseVolEnv << "@" << releaseVolEnvRate
       << "]"   << endl
       << "         "
       <<"Vib:[D:" << delayVibLFO
       << ",P:" << vibLfoToPitch
       << ",D:" << durationVibLFO
       << "]"   << endl;
}

#if 1

void Synthesizer::biQuadSetup()
{
  if (initialFilterQ == 1.0f) {
    a0 = 1.0f;
    a1 = a2 = b1 = b2 = 0.0f;
  }
  else {
    float K = tan(M_PI * initialFilterFc);
    float norm = 1 / (1 + K / initialFilterQ + K * K);

    a0 = K * K * norm;
    a1 = 2 * a0;
    a2 = a0;
    b1 = 2 * (K * K - 1) * norm;
    b2 = (1 - K / initialFilterQ + K * K) * norm;
  }
  z1 = z2 = 0.0f;

  // std::cout << "[Fc:" << initialFilterFc << ",Q:" << initialFilterQ
  //           << ", a0:" << a0 << ",a1:" << a1 << ",a2:" << a2
  //           << ", b1:" << b1 << ",b2:" << b2 << "]"
  //           << std::endl << std:: flush;
}

#else

// Butterworth low-pass parameters
void Synthesizer::biQuadSetup()
{
  if (initialFilterQ == 1.0f) {
    a0 = 1.0f;
    a1 = a2 = b1 = b2 = 0.0f;
  }
  else {
    float alpha = 1 / tan(M_PI * initialFilterFc / config.samplingRate);

    a0 = 1 / (1 + (2 * alpha) + (alpha * alpha));
    a1 = 2 * a0;
    a2 = a0;
    b1 = 2 * a0 * (1 - (alpha * alpha));
    b2 = a0 * (1 - (2 * alpha) + (alpha * alpha));
  }
  z1 = z2 = 0.0f;
}
#endif

bool Synthesizer::transform(buffp dst, buffp src, uint16_t len)
{
  bool endOfSound;

  biQuadFilter(src, len);

  endOfSound = volumeEnvelope(src, len);

  toStereo(dst, src, len);

  pos += len;

  // if (endOfSound) std::cout << "End of Sound" << std::endl;
  // std::cout << "[" << amplVolEnv << "]" << std::endl;

  return endOfSound;
}

float Synthesizer::vibrato(uint32_t pos)
{
  static bool first = true;
  static bool showIt = false;

  if ((vibLfoToPitch  == 0) ||
      (durationVibLFO == 0)) {
    return 1.0f;
  }
  else {
    if (pos < delayVibLFO) {
      return 1.0f;
    }
    else {
      float half = ((float) durationVibLFO) / 2.0f;
      uint32_t loc = (pos - delayVibLFO) % durationVibLFO;
      half = centsToRatio(((half - abs(loc - half))/half) * ((float)vibLfoToPitch));
      assert(half >= 1.0);
      //std::cout << half << "," << std::flush;
      return half;
      if (first) {
        if (loc == 0) {
          showIt = true;
        }
        else {
          first = false;
        }
      }
      else {
        if (loc == 0) {
          showIt = false;
        }
      }
      if (showIt) {
         std::cout << half << "," << std::flush;
      }
      return half;
    }
  }
}
