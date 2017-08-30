#pragma once

#include "Generator.h"

class GeneratorH: public Generator
{
public:
  GeneratorH(FILE* out, const char* guard)
    : Generator(out)
    , _guard(guard)
  {}
  
  virtual void run(const Unit* unit) override;

protected:
  void generateRecord(const Record* record, bool last);
  void generateRecordField(const Record* record, const Record::Field* field, bool last);

  const char* _guard;
};
