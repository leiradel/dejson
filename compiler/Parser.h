#pragma once

#include "Lexer.h"

#include <string>
#include <vector>
#include <setjmp.h>

struct Type
{
  enum
  {
    kScalar,
    kArray,
    kPointer
  };

  int         token;
  std::string name;
  unsigned    attr;
  bool        isSigned;
  bool        isUnsigned;
};

struct Record
{
  struct Field
  {
    std::string name;
    Type        type;
  };

  std::string name;
  std::vector<Field> fields;
};

struct Unit
{
  std::vector<Record> records;
};

class Parser
{
public:
  bool init(const char* buffer, size_t size, Unit* unit, char* error, size_t errorSize);
  void destroy();

  bool parse();

  static bool isNativeType(int id);

protected:
  void match();
  void match(int token);

  void error(const char* format, ...);

  void parseAggregates();
  void parseStruct(Record* structure);
  void parseType(Type* type);
  void parseStructField(Record::Field* field);

  Lexer _lexer;

  Unit* _unit;

  char*  _error;
  size_t _errorSize;

  jmp_buf   _rollback;
  Lookahead _la;
};
