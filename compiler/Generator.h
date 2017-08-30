#pragma once

#include "Lexer.h"
#include "Parser.h"

#include <stdio.h>
#include <stdint.h>

class Generator
{
public:
  Generator(FILE* out);
  
  ~Generator() {}

  virtual void run(const Unit* unit) = 0;

protected:
  void printf(const char* fmt, ...);
  void printfi(const char* fmt, ...);

  void indent()   { _indent++; }
  void unindent() { _indent -= _indent != 0; }

  uint32_t hash(const char* str);

  FILE* _out;
  int   _indent;
};
