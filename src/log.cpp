#include "copyright.h"
#include "mezzo.h"

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

void Log::logIt(const char * level, const char * format, va_list args)
{
  using namespace std;

  static char sbuff[300];

  if (silent) return;

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

  cerr << endl << "PIano abort!" << endl;
  exit(1);
}
