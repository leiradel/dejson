#pragma once

#include <stdint.h>
#include <ctype.h>

struct Str
{
  const char* chars;
  unsigned    length;
};

struct Token
{
  enum
  {
    kEof = 256,

    kIdentifier,
    kStringConstant,
    kNumberConstant,

    kStruct,
    kSigned,
    kUnsigned,
    
    kChar,
    kShort,
    kInt,
    kLong,

    kInt8,
    kInt16,
    kInt32,
    kInt64,
    
    kUint8,
    kUint16,
    kUint32,
    kUint64,

    kFloat,
    kDouble,
    kBool,
    kString,

    kLastToken
  };
};

struct Lookahead
{
  int      token;
  Str      lexeme;
  unsigned line;
};

class Lexer
{
public:
  bool init(const char* buffer, size_t size, char* error, size_t errorSize);
  void destroy();

  bool next(Lookahead* la);

  bool lexeme(char* buffer, size_t size, int token) const;

private:
  bool isspace()  const { return ::isspace(get()); }
  bool isalpha()  const { int k = get(); return k == '_' || ::isalpha(k); }
  bool isalnum()  const { int k = get(); return k == '_' || ::isalnum(k); }
  bool isdigit()  const { return ::isdigit(get()); }
  bool isxdigit() const { return ::isxdigit(get()); }
  bool isoctal()  const { int k = get(); return k >= '0' && k <= '7'; }
  
  int get() const
  {
    return _buffer < _end ? *_buffer : -1;
  }

  void skip()
  {
    _line += *_buffer == '\n';
    _buffer += _buffer < _end;
  }

  bool skipEscapeSeq();
  bool skipComment();

  void formatChar(char* buffer, size_t size, int k) const;
  bool error(const char* format, ...) const;

  const uint8_t* _buffer;
  const uint8_t* _end;

  char*  _error;
  size_t _errorSize;

  unsigned _line;
};
