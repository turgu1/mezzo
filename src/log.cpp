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

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "mezzo.h"

void Log::logIt(const char * level, const char * format, va_list args)
{
  using namespace std;

  static char sbuff[300];

  if (config.silent) return;

  vsnprintf(sbuff, 299, format, args);

  cerr << level << " : " << sbuff << endl;
}

void Log::DEBUG(const char * format, ...)
{
  va_list args;

  va_start(args, format);
  logIt("DEBUG", format, args);
}

void Log::INFO(const char * format, ...)
{
  va_list args;

  va_start(args, format);
  logIt("INFO", format, args);
}

void Log::WARNING(const char * format, ...)
{
  va_list args;

  va_start(args, format);
  logIt("WARNING", format, args);
}

void Log::ERROR(const char * format, ...)
{
  va_list args;

  va_start(args, format);
  logIt("ERROR", format, args);
}

void Log::FATAL(const char * format, ...)
{
  va_list args;

  using namespace std;

  va_start(args, format);
  logIt("FATAL", format, args);

  cerr << endl << "Mezzo abort!" << endl;
  exit(1);
}
