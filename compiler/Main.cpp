#include "GeneratorC.h"
#include "GeneratorH.h"
#include "Parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

static bool parse(Unit* unit, const char* input, char* source, size_t size)
{
  Parser parser;
  char error[1024];

  if (!parser.init(source, size, unit, error, sizeof(error)))
  {
    return false;
  }

  if (!parser.parse())
  {
    fprintf(stderr, "%s:%s\n", input, error);
    return false;
  }

  return true;
}

static void generate(const Unit* unit, const char* name)
{
  char source[1024];
  snprintf(source, sizeof(source), "%s.c", name);

  char header[1024];
  snprintf(header, sizeof(header), "%s.h", name);

  char guard[1024];
  snprintf(guard, sizeof(guard), "__%s__", header);

  for (int i = 0; guard[i] != 0; i++)
  {
    if (isalnum(guard[i]))
    {
      guard[i] = toupper(guard[i]);
    }
    else
    {
      guard[i] = '_';
    }
  }

  FILE* file = fopen(header, "w");

  GeneratorH genh(file, guard);
  genh.run(unit);

  fclose(file);

  file = fopen(source, "w");

  GeneratorC genc(file, header);
  genc.run(unit);

  fclose(file);
}

int main(int argc, const char* argv[])
{
  const char* name = NULL;
  const char* input = NULL;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--output"))
    {
      if (++i == argc)
      {
        fprintf(stderr, "Missing argument to --output\n");
        return 1;
      }

      name = argv[i];
    }
    else
    {
      input = argv[i];
    }
  }

  if (name == NULL)
  {
    fprintf(stderr, "Missing output name\n");
    return 1;
  }

  if (input == NULL)
  {
    fprintf(stderr, "Missing input file name\n");
    return 1;
  }

  char* source;
  size_t size;

  {
    struct stat buf;

    if (stat(input, &buf) != 0)
    {
      fprintf(stderr, "Error getting the size of \"%s\": %s\n", name, strerror(errno));
      return 1;
    }

    size = buf.st_size;
    source = (char*)malloc(size);

    if (source == NULL)
    {
      fprintf(stderr, "Out of memory\n");
      return 1;
    }

    FILE* file = fopen(input, "rb");

    if (file == NULL)
    {
      fprintf(stderr, "Error opening \"%s\": %s\n", name, strerror(errno));
      return 1;
    }

    if (fread((void*)source, 1, size, file) != size)
    {
      fclose(file);
      fprintf(stderr, "Error reading from \"%s\"\n", name);
      return 1;
    }

    fclose(file);
  }

  Unit unit;

  if (!parse(&unit, input, source, size))
  {
    return 1;
  }

  generate(&unit, name);
  return 0;
};
