#include "copyright.h"

#ifndef LOG_H
#define LOG_H

#include <cstdarg>

class Log {

 private:
  void logIt(const char * level, const char * format, va_list args);

 public:
  void   DEBUG(const char * fmt, ...);
  void    INFO(const char * fmt, ...);
  void WARNING(const char * fmt, ...);
  void   ERROR(const char * fmt, ...);
  void   FATAL(const char * fmt, ...);
};

#endif

