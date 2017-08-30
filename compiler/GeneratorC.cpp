#include "GeneratorC.h"

#include "dejson.h"

void GeneratorC::run(const Unit* unit)
{
  printf("#include \"%s\"\n\n", _header);

  const size_t numRecords = unit->records.size();
  
  for (size_t i = 0; i < numRecords; i++)
  {
    generateRecord(&unit->records[i], i == numRecords - 1);
  }

  printfi("const dejson_record_meta_t* dejson_resolve_record(uint32_t hash)\n");
  printfi("{\n");
  indent();

  printfi("switch (hash)\n");
  printfi("{\n");
  indent();
  
  for (size_t i = 0; i < numRecords; i++)
  {
    const Record* record = &unit->records[i];
    printfi("case 0x%08xU: return &g_Meta%s;\n", hash(record->name.c_str()), record->name.c_str());
  }

  printfi("default: return NULL;\n");

  unindent();
  printfi("}\n");
  unindent();
  printfi("}\n");
}

void GeneratorC::generateRecord(const Record* record, bool last)
{
  printfi("static const dejson_record_field_meta_t s_fieldMeta%s[] =\n", record->name.c_str(), record->fields.size());
  printfi("{\n");
  indent();

  const size_t numFields = record->fields.size();
  
  for (size_t i = 0; i < numFields; i++)
  {
    generateRecordField(record, &record->fields[i], i == numFields - 1);
  }

  unindent();
  printfi("};\n\n");

  printfi("const dejson_record_meta_t g_Meta%s =\n", record->name.c_str(), record->fields.size());
  printfi("{\n");
  indent();

  printfi("/* fields     */ s_fieldMeta%s,\n", record->name.c_str());
  printfi("/* name_hash  */ 0x%08xU, /* %s */\n", hash(record->name.c_str()), record->name.c_str());
  printfi("/* size       */ sizeof(%s),\n", record->name.c_str());
  printfi("/* alignment  */ DEJSON_ALIGNOF(%s),\n", record->name.c_str());
  printfi("/* num_fields */ %lu\n", numFields);

  unindent();
  printfi("};\n\n");
}

const char* GeneratorC::dejsonType(const Type* type)
{
  if (type->isUnsigned)
  {
    switch (type->token)
    {
    case Token::kChar:  return "DEJSON_TYPE_UCHAR";
    case Token::kShort: return "DEJSON_TYPE_USHORT";
    case Token::kInt:   return "DEJSON_TYPE_UINT";
    case Token::kLong:  return "DEJSON_TYPE_ULONG";
    }
  }
  else
  {
    switch (type->token)
    {
    case Token::kChar:       return "DEJSON_TYPE_CHAR";
    case Token::kShort:      return "DEJSON_TYPE_SHORT";
    case Token::kInt:        return "DEJSON_TYPE_INT";
    case Token::kLong:       return "DEJSON_TYPE_LONG";
    case Token::kInt8:       return "DEJSON_TYPE_INT8";
    case Token::kInt16:      return "DEJSON_TYPE_INT16";
    case Token::kInt32:      return "DEJSON_TYPE_INT32";
    case Token::kInt64:      return "DEJSON_TYPE_INT64";
    case Token::kUint8:      return "DEJSON_TYPE_UINT8";
    case Token::kUint16:     return "DEJSON_TYPE_UINT16";
    case Token::kUint32:     return "DEJSON_TYPE_UINT32";
    case Token::kUint64:     return "DEJSON_TYPE_UINT64";
    case Token::kFloat:      return "DEJSON_TYPE_FLOAT";
    case Token::kDouble:     return "DEJSON_TYPE_DOUBLE";
    case Token::kBool:       return "DEJSON_TYPE_BOOL";
    case Token::kString:     return "DEJSON_TYPE_STRING";
    case Token::kIdentifier: return "DEJSON_TYPE_RECORD";
    }
  }

  return "@";
}

void GeneratorC::generateRecordField(const Record* record, const Record::Field* field, bool last)
{
  const char* flags = "0";

  if (field->type.attr == Type::kArray)
  {
    flags = "DEJSON_FLAG_ARRAY";
  }
  else if (field->type.attr == Type::kPointer)
  {
    flags = "DEJSON_FLAG_POINTER";
  }

  printfi("{\n");
  indent();

  printfi("/* name_hash */ 0x%08xU, /* %s */\n", hash(field->name.c_str()), field->name.c_str());

  if (field->type.token == Token::kIdentifier)
  {
    printfi("/* type_hash */ 0x%08xU, /* %s */\n", hash(field->type.name.c_str()), field->type.name.c_str());
  }
  else
  {
    printfi("/* type_hash */ 0x00000000U,\n");
  }

  printfi("/* offset    */ DEJSON_OFFSETOF(%s, %s),\n", record->name.c_str(), field->name.c_str());
  printfi("/* type      */ %s,\n", dejsonType(&field->type));
  printfi("/* flags     */ %s\n", flags);

  unindent();
  printfi("}%s\n", last ? "" : ",");
}
