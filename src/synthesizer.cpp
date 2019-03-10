// Notice
// ------
//
// This file is part of the Mezzo SoundFont2 Sampling Based Synthesizer:
//
//     https://github.com/turgu1/mezzo
//
// Simplified BSD License
// ----------------------
//
// Copyright (c) 2018, Guy Turcotte
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//

#include "mezzo.h"

#include <math.h>
#include <iomanip>

#define SetOrAdd(op,x,y)                                         \
  if (type == init) x.set##y(gens->genAmount.shAmount);          \
  else if (type == set) x.set##y(gens->genAmount.shAmount);      \
  else if (type == adjust) x.addTo##y(gens->genAmount.shAmount); \
  break

static inline bool check(const generatorDescriptor & gen, const genAmountType & value)
{
  bool result = true;
  if (gen.checkRange) {
    if (gen.valueType == 1) {
      result = !((gen.minRange > value.shAmount) || (value.shAmount > gen.highRange));
    }
    else if (gen.valueType == 2) {
      result = !((gen.minRange > value.wAmount) || (value.wAmount > gen.highRange));
    }
  }
  //if (!result) std::cout << gen.name << " is out of range: " << value.shAmount << std::endl;
  return result;
}

void Synthesizer::setGens(sfGenList * gens, uint8_t genCount, setGensType type)
{
  Synthesizer & me = *this;

  while (genCount--) {
    if (!check(generatorsDesc[gens->sfGenOper], gens->genAmount)) { gens++; continue; }
    SFGenerator op = gens->sfGenOper;
    switch (op) {
      case sfGenOper_startAddrsOffset:
        start += gens->genAmount.shAmount;
        break;
      case sfGenOper_endAddrsOffset:
        end += gens->genAmount.shAmount;
        break;
      case sfGenOper_startloopAddrsOffset:
        //std::cout << "startLoop:" << startLoop << "/ " << gens->genAmount.shAmount << std::endl;
        startLoop += gens->genAmount.shAmount;
        break;
      case sfGenOper_endloopAddrsOffset:
        endLoop += gens->genAmount.shAmount;
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
      case  sfGenOper_sampleModes:
        //std::cout << "SampleModes:" << gens->genAmount.wAmount << std::endl;
        loop = (gens->genAmount.wAmount & 1) != 0;
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
      case  sfGenOper_keynum:
        keynum = gens->genAmount.wAmount;
        break;
      case  sfGenOper_coarseTune:
        transpose = (type == set) ?
          gens->genAmount.shAmount :
          (transpose + gens->genAmount.shAmount);
        break;
      case  sfGenOper_fineTune:
        fineTune = (type == set) ?
          gens->genAmount.shAmount :
          (fineTune + gens->genAmount.shAmount);
        break;

      // ----- Volume envelope -----

      case  sfGenOper_delayVolEnv:         SetOrAdd(op, volEnvelope, Delay        );
      case  sfGenOper_attackVolEnv:        SetOrAdd(op, volEnvelope, Attack       );
      case  sfGenOper_holdVolEnv:          SetOrAdd(op, volEnvelope, Hold         );
      case  sfGenOper_decayVolEnv:         SetOrAdd(op, volEnvelope, Decay        );
      case  sfGenOper_releaseVolEnv:       SetOrAdd(op, volEnvelope, Release      );
      case  sfGenOper_sustainVolEnv:       SetOrAdd(op, volEnvelope, Sustain      );
      case  sfGenOper_initialAttenuation:  SetOrAdd(op, me,          Attenuation  );
      case  sfGenOper_keynumToVolEnvHold:  SetOrAdd(op, volEnvelope, KeynumToHold );
      case  sfGenOper_keynumToVolEnvDecay: SetOrAdd(op, volEnvelope, KeynumToDecay);

      case  sfGenOper_delayModEnv:         SetOrAdd(op, modEnvelope, Delay        );
      case  sfGenOper_attackModEnv:        SetOrAdd(op, modEnvelope, Attack       );
      case  sfGenOper_holdModEnv:          SetOrAdd(op, modEnvelope, Hold         );
      case  sfGenOper_decayModEnv:         SetOrAdd(op, modEnvelope, Decay        );
      case  sfGenOper_sustainModEnv:       SetOrAdd(op, modEnvelope, Sustain      );
      case  sfGenOper_releaseModEnv:       SetOrAdd(op, modEnvelope, Release      );
      case  sfGenOper_keynumToModEnvHold:  SetOrAdd(op, modEnvelope, KeynumToHold );
      case  sfGenOper_keynumToModEnvDecay: SetOrAdd(op, modEnvelope, KeynumToDecay);

      // Low-Pass BiQuad Filter
      case  sfGenOper_initialFilterFc:     SetOrAdd(op, biQuad, InitialFc);
      case  sfGenOper_initialFilterQ:      SetOrAdd(op, biQuad, InitialQ );

      // Vibrato
      case  sfGenOper_vibLfoToPitch:       SetOrAdd(op, vib, Pitch       );
      case  sfGenOper_delayVibLFO:         SetOrAdd(op, vib, Delay       );
      case  sfGenOper_freqVibLFO:          SetOrAdd(op, vib, Frequency   );

      case  sfGenOper_modLfoToPitch:
      case  sfGenOper_modLfoToFilterFc:
      case  sfGenOper_modLfoToVolume:
      case  sfGenOper_delayModLFO:
      case  sfGenOper_freqModLFO:

      case  sfGenOper_modEnvToPitch:
      case  sfGenOper_modEnvToFilterFc:

      case  sfGenOper_chorusEffectsSend:
      case  sfGenOper_reverbEffectsSend:

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
  start            = sample->getStart();
  end              = sample->getEnd();
  startLoop        = sample->getStartLoop();
  endLoop          = sample->getEndLoop();
  sampleRate       = sample->getSampleRate();
  rootKey          = sample->getPitch();
  correctionFactor = centsToRatio(sample->getCorrection());
  loop             = false;

  pan              =     0;
  velocity         =    -1;
  keynum           =    -1;
  transpose        =     0;
  fineTune         =     0;
  attenuation      =  1.0f;
}

void Synthesizer::completeParams(uint8_t note)
{
  using namespace std;

  if (loop) {
    sizeSample = endLoop;
  }
  else {
    sizeSample = end - start;
  }
  sizeLoop  = endLoop - startLoop;

  volEnvelope.setup(note);
  modEnvelope.setup(note);
  vib.setup(note);
  //biQuad.setup();

  //std::cout << correctionFactor << " / " << fineTune << " / " << centsToRatio(fineTune) << std::endl << std::flush;
  correctionFactor *= centsToRatio(fineTune);

  pos = 0;

  // Stereo panning left/right

  float fpan = pan * (1.0f / 1000.0f);

  const float prop  = M_SQRT2 * 0.5f;
  const float angle = ((float) fpan) * M_PI;

  left  = prop * (Utils::lowCos(angle) - Utils::lowSin(angle));
  right = prop * (Utils::lowCos(angle) + Utils::lowSin(angle));
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
       << " looping:"     << (const char *) (loop ? "yes" : "no")
       << " startLoop:"   << startLoop
       << " endLoop:"     << endLoop
       << " pan:"         << pan
       << " sizeSample:"  << sizeSample
       << " sizeLoop:"    << sizeLoop
       << " correction:"  << fixed << setw(7) << setprecision(5) << correctionFactor
       << " Att:"         << fixed << setw(7) << setprecision(5) << attenuation << "]" << endl;

  volEnvelope.showStatus(spaces + 4);
          vib.showStatus(spaces + 4);
       biQuad.showStatus(spaces + 4);
}
