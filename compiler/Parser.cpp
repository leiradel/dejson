#include "Parser.h"
#include "Lexer.h"
#include "Generator.h"

#include <stdarg.h>
#include <stdlib.h>

bool Parser::init(const char* buffer, size_t size, Unit* unit, char* error, size_t errorSize)
{
  if (!_lexer.init(buffer, size, error, errorSize))
  {
    return false;
  }

  _unit = unit;
  _error = error;
  _errorSize = errorSize;

  return true;
}

void Parser::destroy()
{
  _lexer.destroy();
}

bool Parser::parse()
{
  if (setjmp(_rollback) != 0)
  {
    return false;
  }

  match();
  parseAggregates();
  return true;
}

bool Parser::isNativeType(int id)
{
  switch (id)
  {
  case Token::kFloat:
  case Token::kDouble:
  case Token::kInt:
  case Token::kUnsigned:
  case Token::kChar:
  case Token::kLong:
  case Token::kInt8:
  case Token::kInt16:
  case Token::kInt32:
  case Token::kInt64:
  case Token::kUint8:
  case Token::kUint16:
  case Token::kUint32:
  case Token::kUint64:
  case Token::kBool:
  case Token::kString:
    return true;
  
  default:
    return false;
  }
}

void Parser::match()
{
  if (!_lexer.next(&_la))
  {
    longjmp(_rollback, 1);
  }
}

void Parser::match(int token)
{
  if (_la.token != token)
  {
    char lexeme[256];
    _lexer.lexeme(lexeme, sizeof(lexeme), token);
    error("%s expected", lexeme);
  }

  match();
}

void Parser::error(const char* format, ...)
{
  va_list args;
  va_start(args, format);

  char msg[4096];
  vsnprintf(msg, sizeof(msg), format, args);

  va_end(args);

  snprintf(_error, _errorSize, "%u: %s", _la.line, msg);
  longjmp(_rollback, 1);
}

void Parser::parseAggregates()
{
  while (_la.token == Token::kStruct)
  {
    Record record;
    parseStruct(&record);
    _unit->records.push_back(record);
  }
}

void Parser::parseStruct(Record* record)
{
  match(Token::kStruct);

  record->name = std::string(_la.lexeme.chars, _la.lexeme.length);
  match(Token::kIdentifier);

  match('{');

  for (;;)
  {
    Record::Field field;
    parseStructField(&field);
    record->fields.push_back(field);

    if (_la.token == '}')
    {
      break;
    }
  }

  match('}');
  match(';');
}

void Parser::parseType(Type* type)
{
  type->isSigned = false;
  type->isUnsigned = false;

  if (_la.token == Token::kSigned)
  {
    type->isSigned = true;
    match();
  }
  else if (_la.token == Token::kUnsigned)
  {
    type->isUnsigned = true;
    match();
  }

  if (_la.token == Token::kSigned)
  {
    if (type->isSigned)
    {
      error("duplicate 'signed'");
    }
    else if (type->isUnsigned)
    {
      error("'signed' and 'unsigned' specified together");
    }
  }
  else if (_la.token == Token::kUnsigned)
  {
    if (type->isUnsigned)
    {
      error("duplicate 'unsigned'");
    }
    else if (type->isSigned)
    {
      error("'signed' and 'unsigned' specified together");
    }
  }

  type->token = _la.token;
  type->name = std::string(_la.lexeme.chars, _la.lexeme.length);

  switch (_la.token)
  {
  case Token::kChar:
  case Token::kShort:
  case Token::kInt:
  case Token::kLong:
    match();
    break;

  case Token::kInt8:
  case Token::kInt16:
  case Token::kInt32:
  case Token::kInt64:
  case Token::kUint8:
  case Token::kUint16:
  case Token::kUint32:
  case Token::kUint64:
  case Token::kFloat:
  case Token::kDouble:
  case Token::kBool:
  case Token::kString:
    if (type->isSigned || type->isUnsigned)
    {
      error("'signed' or 'unsigned' invalid for '%.*s'", _la.lexeme.length, _la.lexeme.chars);
    }

    match();
    break;

  case Token::kIdentifier:
    if (type->isSigned || type->isUnsigned)
    {
      type->token = Token::kInt;
      type->name = "int";
    }
    else
    {
      match();
    }

    break;
  
  default:
    if (type->isSigned || type->isUnsigned)
    {
      type->token = Token::kInt;
      type->name = "int";
    }
    else
    {
      error("type or identifier expected");
    }
    
    break;
  }

  if (_la.token == '*')
  {
    type->attr = Type::kPointer;
    match();
  }
  else
  {
    type->attr = Type::kScalar;
  }
}

void Parser::parseStructField(Record::Field* field)
{
  parseType(&field->type);
  field->name = std::string(_la.lexeme.chars, _la.lexeme.length);
  match(Token::kIdentifier);

  if (_la.token == '[')
  {
    if (field->type.attr == Type::kScalar)
    {
      match();
      match(']');

      field->type.attr = Type::kArray;
    }
    else
    {
      error("Arrays of pointers are not supported");
    }
  }

  match(';');
}
