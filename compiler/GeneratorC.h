#pragma once

#include "Generator.h"

class GeneratorC: public Generator
{
public:
  GeneratorC(FILE* out, const char* header)
    : Generator(out)
    , _header(header)
  {}

  virtual void run(const Unit* unit) override;

protected:
  const char* dejsonType(const Type* type);
  void        generateRecord(const Record* record, bool last);
  void        generateRecordField(const Record* record, const Record::Field* field, bool last);
  void        generateRecordFieldNew(const Record* record, const Record::Field* field, bool last);

  const char* _header;
};
