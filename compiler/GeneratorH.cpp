#include "GeneratorH.h"

#include "dejson.h"

#include <vector>
#include <algorithm>

const size_t getSize(const Record::Field* field)
{
  static const size_t sizes[] =
  {
    sizeof(char), sizeof(short), sizeof(int), sizeof(long),
    sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t),
    sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t),
    sizeof(float), sizeof(double), sizeof(char), sizeof(dejson_string_t)
  };

  if (field->type.attr == Type::kScalar)
  {
    if (field->type.token == Token::kIdentifier)
    {
      return 5 << 8;
    }
    else
    {
      int ndx = field->type.token - Token::kChar;
      return sizes[ndx] << 8 | ndx;
    }
  }
  else
  {
    return sizeof(void*) << 8;
  }
}

static bool compareFields(const Record::Field* f1, const Record::Field* f2)
{
  return getSize(f1) > getSize(f2);
}

void GeneratorH::run(const Unit* unit)
{
  printf("#ifndef %s\n", _guard);
  printf("#define %s\n\n", _guard);
  printf("#include <dejson.h>\n");
  printf("#include <stdint.h>\n\n");

  const size_t count = unit->records.size();

  for (size_t i = 0; i < count; i++)
  {
    generateRecord(&unit->records[i], i == count - 1);
  }

  printf("const dejson_record_meta_t* dejson_resolve_record(uint32_t hash);\n\n");
  printf("#endif /* %s */\n", _guard);
}

void GeneratorH::generateRecord(const Record* record, bool last)
{
  (void)last;

  std::vector<const Record::Field*> fields;

  size_t count = record->fields.size();
  
  for (size_t i = 0; i < count; i++)
  {
    fields.push_back(&record->fields[i]);
  }

  std::sort(fields.begin(), fields.end(), compareFields);

  printfi("typedef struct\n");
  printfi("{\n");
  indent();

  for (size_t i = 0; i < count; i++)
  {
    generateRecordField(record, fields[i], i == count - 1);
  }

  unindent();
  printfi("}\n");
  printfi("%s;\n\n", record->name.c_str());

  printfi("extern const dejson_record_meta_t g_Meta%s;\n\n", record->name.c_str(), record->fields.size());
}

void GeneratorH::generateRecordField(const Record* record, const Record::Field* field, bool last)
{
  (void)record;

  const char* sig = "";

  if (field->type.isSigned)
  {
    sig = "signed ";
  }
  else if (field->type.isUnsigned)
  {
    sig = "unsigned ";
  }
  
  const char* type = field->type.name.c_str();

  if (field->type.token == Token::kString)
  {
    type = "dejson_string_t";
  }
  else if (field->type.token == Token::kBool)
  {
    type = "char";
  }

  if (field->type.attr == Type::kArray)
  {
    printfi("dejson_array_t %s; /* %s */\n", field->name.c_str(), type);
  }
  else
  {
    printfi("%s%s%s %s;\n", sig, type, field->type.attr == Type::kPointer ? "*" : "", field->name.c_str());
  }
}
