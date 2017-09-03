#include "Lexer.h"
#include "Tokens.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

bool Lexer::init(const char* buffer, size_t size, char* error, size_t errorSize)
{
  _buffer = (uint8_t*)buffer;
  _end = _buffer + size;

  _error = error;
  _errorSize = errorSize;

  _line = 1;
  return true;
}

void Lexer::destroy()
{
}

bool Lexer::next(Lookahead* la)
{
  for (;;)
  {
    if (get() == -1)
    {
      la->token = Token::kEof;
      la->lexeme.chars = "<eof>";
      la->lexeme.length = 5;
      la->line = _line;
      return true;
    }
    else if (!isspace())
    {
      break;
    }

    skip();
  }

  const uint8_t* start = _buffer;
  la->lexeme.chars = (char*)start;
  la->line = _line;

  if (isdigit())
  {
    bool isInt = true;

    if (get() == '0')
    {
      skip();

      if (tolower(get()) == 'x')
      {
        do
        {
          skip();
        }
        while (isxdigit());

        if (_buffer - start < 3)
        {
          return error("No digits in hexadecimal constant");
        }

        goto gotnumber;
      }
      else if (isoctal())
      {
        do
        {
          skip();
        }
        while (isoctal());

        goto gotnumber;
      }

      // Fall through and read a decimal number
    }

    while (isdigit())
    {
      skip();
    }

    if (get() == '.')
    {
      isInt = false;
      skip();

      while (isdigit())
      {
        skip();
      }
    }

    if (tolower(get()) == 'e')
    {
      isInt = false;
      skip();

      if (get() == '+' || get() == '-')
      {
        skip();
      }

      if (!isdigit())
      {
        return error("No digits in exponent");
      }

      do
      {
        skip();
      }
      while (isdigit());
    }

  gotnumber:;
    uint32_t suffix = 0;

    while (isalpha())
    {
      suffix = suffix << 8 | get();
      skip();
    }

    if (isInt)
    {
      switch (suffix)
      {
      case 0:
      case 'u':
      case 'u' <<  8 | 'l':
      case 'u' << 16 | 'l' << 8 | 'l':
      case 'l':
      case 'l' <<  8 | 'u':
      case 'l' <<  8 | 'l':
      case 'l' << 16 | 'l' << 8 | 'u':
        break;
      
      default:
        return error("Invalid integer suffix");
      }
    }
    else
    {
      switch (suffix)
      {
      case 0:
      case 'f':
      case 'l':
        break;
      
      default:
        return error("Invalid integer suffix");
      }
    }

    la->token = Token::kNumberConstant;
    la->lexeme.length = _buffer - start;
    return true;
  }

  if (isalpha())
  {
    do
    {
      skip();
    }
    while (isalnum());

    const Keyword* keyword = Perfect_Hash::in_word_set((char*)start, _buffer - start);

    la->token = (keyword != NULL) ? keyword->token : Token::kIdentifier;
    la->lexeme.length = _buffer - start;
    return true;
  }

  if (get() == '"')
  {
    skip();

    for (;;)
    {
      int k = get();
      skip();

      if (k == '"')
      {
        skip();
        break;
      }
      else if (k == -1)
      {
        return error("Unterminated string");
      }
      else if (k == '\\')
      {
        if (!skipEscapeSeq())
        {
          return false;
        }
      }
    }

    la->token = Token::kString;
    la->lexeme.chars++;
    la->lexeme.length = _buffer - start;
    return true;
  }

  la->token = get();

  switch (la->token)
  {
  case '{':
  case '}':
  case '[':
  case ']':
  case ';':
  case '<':
  case '>':
  case '*':
    skip();
    la->lexeme.length = 1;
    return true;

  case '/':
    skip();

    if (get() == '/' || get() == '*')
    {
      skipComment();
      return next(la);
    }

    return error("Invalid character in input: '/'", la->token);

  }

  char str[16];
  formatChar(str, sizeof(str), la->token);
  return error("Invalid character in input: %s", str);
}

bool Lexer::skipEscapeSeq()
{
  skip();
  int k = get();
  skip();

  switch (k)
  {
  case 'a':
  case 'b':
  case 'f':
  case 'n':
  case 'r':
  case 't':
  case 'v':
  case '\\':
  case '\'':
  case '"':
  case '?':
    return true;
  
  case 'x':
    if (!isxdigit())
    {
      return error("\\x used with no following hex digits");
    }

    do
    {
      skip();
    }
    while (isxdigit());

    return true;
  
  case 'u':
  case 'U':
    for (int i = 4 + 4 * (k == 'U'); i != 0; i--)
    {
      if (!isxdigit())
      {
        return error("\\%c needs %d hexadecimal digits", k, 4 + 4 * (k == 'U'));
      }

      skip();
    }

    return true;
  
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
    while (isoctal())
    {
      skip();
    }

    return true;
  }

  char str[16];
  formatChar(str, sizeof(str), k);
  return error("Unknown escape sequence: %s", str);
}

bool Lexer::lexeme(char* buffer, size_t size, int token) const
{
  static const char* tokens[] =
  {
    "<eof>", "identifier", "string", "number", "struct", "signed",
    "unsigned", "char", "short", "int", "long", "int8_t", "int16_t",
    "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "float", "double", "bool", "string"
  };

  if (token < Token::kEof)
  {
    formatChar(buffer, size, token);
  }
  else if (token < Token::kLastToken)
  {
    snprintf(buffer, size, "%s", tokens[token - Token::kEof]);
  }
  else
  {
    snprintf(buffer, size, "unknown (%d)", token);
    return false;
  }

  return true;
}

bool Lexer::skipComment()
{
  if (get() == '/')
  {
    do
    {
      skip();
    }
    while (get() != '\n' && get() != -1);

    return true;
  }
  else if (get() == '*')
  {
    skip();

    do
    {
      while (get() != '*')
      {
        if (get() == -1)
        {
          return error ("Unterminated comment");
        }

        skip();
      }

      skip();
    }
    while (get() != '/');

    skip();
    return true;
  }

  // Shouldn't happen
  return error("Not a comment");
}

void Lexer::formatChar(char* buffer, size_t size, int k) const
{
  if (isprint(k))
  {
    snprintf(buffer, size, "'%c'", k);
  }
  else
  {
    snprintf(buffer, size, "'\\%c%c%c", (k >> 6) + '0', ((k >> 3) & 7) + '0', (k & 7) + '0');
  }
}

bool Lexer::error(const char* format, ...) const
{
  va_list args;
  va_start(args, format);

  vsnprintf(_error, _errorSize, format, args);

  va_end(args);
  return false;
}
