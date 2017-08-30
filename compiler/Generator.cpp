#include "Generator.h"

#include <stdarg.h>

Generator::Generator(FILE* out)
{
  _out = out;
  _indent = 0;
}

void Generator::printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(_out, fmt, args);
  va_end(args);
}

void Generator::printfi(const char* fmt, ...)
{
  for (int i = _indent; i != 0; --i)
  {
    fprintf(_out, "  ");
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(_out, fmt, args);
  va_end(args);
}

uint32_t Generator::hash(const char* str)
{
  uint32_t hash = 5381;

  if (*str != 0)
  {
    do
    {
      hash = hash * 33 + (uint8_t)*str++;
    }
    while (*str != 0);
  }

  return hash;
}
