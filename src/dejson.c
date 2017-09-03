#include <dejson.h>

#include <setjmp.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <errno.h>

typedef struct
{
  const uint8_t* json;
  uintptr_t      buffer;
  int            counting;
  jmp_buf        rollback;
}
dejson_state_t;

static void* dejson_alloc(dejson_state_t* state, size_t size, size_t alignment)
{
  state->buffer = (state->buffer + alignment - 1) & ~(alignment - 1);
  void* ptr = (void*)state->buffer;
  state->buffer += size;
  return ptr;
}

static void dejson_skip_spaces(dejson_state_t* state)
{
  if (isspace(*state->json))
  {
    do
    {
      state->json++;
    }
    while (isspace(*state->json));
  }
}

static size_t dejson_skip_string(dejson_state_t*);
static void dejson_skip_value(dejson_state_t*);

static void dejson_skip_object(dejson_state_t* state)
{
  state->json++;
  dejson_skip_spaces(state);
  
  while (*state->json != '}')
  {
    dejson_skip_string(state);
    dejson_skip_spaces(state);

    if (*state->json != ':')
    {
      longjmp(state->rollback, DEJSON_INVALID_VALUE);
    }

    state->json++;
    dejson_skip_spaces(state);
    dejson_skip_value(state);
    dejson_skip_spaces(state);

    if (*state->json != ',')
    {
      break;
    }

    state->json++;
    dejson_skip_spaces(state);
  }

  if (*state->json != '}')
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json++;
}

static size_t dejson_skip_array(dejson_state_t* state)
{
  size_t count = 0;
  state->json++;
  dejson_skip_spaces(state);
  
  while (*state->json != ']')
  {
    dejson_skip_value(state);
    dejson_skip_spaces(state);

    count++;

    if (*state->json != ',')
    {
      break;
    }

    state->json++;
    dejson_skip_spaces(state);
  }

  if (*state->json != ']')
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json++;
  return count;
}

static void dejson_skip_number(dejson_state_t* state)
{
  errno = 0;

  char* end;
  double result = strtod((const char*)state->json, &end);

  if ((result == 0.0 && end == (const char*)state->json) || errno == ERANGE)
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json = (uint8_t*)end;
}

static void dejson_skip_boolean(dejson_state_t* state)
{
  const uint8_t* json = state->json;

  if (json[0] == 't' && json[1] == 'r' && json[2] == 'u' && json[3] == 'e' && !isalpha(json[4]))
  {
    state->json += 4;
  }
  else if (json[0] == 'f' && json[1] == 'a' && json[2] == 'l' && json[3] == 's' && json[4] == 'e' && !isalpha(json[5]))
  {
    state->json += 5;
  }
  else
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }
}

static size_t dejson_skip_string(dejson_state_t* state)
{
  const uint8_t* aux = state->json + 1;
  size_t length = 0;
  
  if (*aux !='"')
  {
    do
    {
      length++;

      if (*aux++ == '\\')
      {
        char digits[5];
        uint32_t utf32;

        switch (*aux++)
        {
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
          break;
        
        case 'u':
          if (!isxdigit(aux[0] || !isxdigit(aux[1]) || !isxdigit(aux[2]) || !isxdigit(aux[3])))
          {
            longjmp(state->rollback, DEJSON_INVALID_ESCAPE);
          }

          digits[0] = aux[0];
          digits[1] = aux[1];
          digits[2] = aux[2];
          digits[3] = aux[3];
          digits[4] = 0;
          aux += 4;
          
          utf32 = strtoul(digits, NULL, 16);

          if (utf32 < 0x80U)
          {
          }
          else if (utf32 < 0x800U)
          {
            length++;
          }
          else if (utf32 < 0x10000U)
          {
            length += 2;
          }
          else if (utf32 < 0x200000U)
          {
            length += 3;
          }
          else
          {
            longjmp(state->rollback, DEJSON_INVALID_ESCAPE);
          }

          break;
        
        default:
          longjmp(state->rollback, DEJSON_INVALID_ESCAPE);
        }
      }
    }
    while (*aux != '"');
  }

  state->json = aux + 1;
  return length + 1;
}

static void dejson_skip_null(dejson_state_t* state)
{
  const uint8_t* json = state->json;

  if (json[0] == 'n' && json[1] == 'u' && json[2] == 'l' && json[3] == 'l' && !isalpha(json[4]))
  {
    state->json += 4;
  }
  else
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }
}

static void dejson_skip_value(dejson_state_t* state)
{
  switch (*state->json)
  {
  case '{':
    dejson_skip_object(state);
    break;

  case '[':
    dejson_skip_array(state);
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '-':
    dejson_skip_number(state);
    break;

  case 't':
  case 'f':
    dejson_skip_boolean(state);
    break;

  case '"':
    dejson_skip_string(state);
    break;

  case 'n':
    dejson_skip_null(state);
    break;

  default:
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  dejson_skip_spaces(state);
}

static int64_t dejson_get_int64(dejson_state_t* state, int64_t min, int64_t max)
{
  errno = 0;

  char* end;
  long long result = strtoll((const char*)state->json, &end, 10);

  if ((result == 0 && end == (const char*)state->json) || errno == ERANGE || result < min || result > max)
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json = (uint8_t*)end;
  return result;
}

static uint64_t dejson_get_uint64(dejson_state_t* state, uint64_t max)
{
  errno = 0;

  char* end;
  unsigned long long result = strtoull((char*)state->json, &end, 10);

  if ((result == 0 && end == (const char*)state->json) || errno == ERANGE || result > max)
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json = (uint8_t*)end;
  return result;
}

static double dejson_get_double(dejson_state_t* state, double min, double max)
{
  errno = 0;

  char* end;
  double result = strtod((const char*)state->json, &end);

  if ((result == 0.0 && end == (const char*)state->json) || errno == ERANGE || result < min || result > max)
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  state->json = (uint8_t*)end;
  return result;
}

static void dejson_parse_char(dejson_state_t* state, void* data)
{
  *(char*)data = dejson_get_int64(state, CHAR_MIN, CHAR_MAX);
}

static void dejson_parse_uchar(dejson_state_t* state, void* data)
{
  *(unsigned char*)data = dejson_get_uint64(state, UCHAR_MAX);
}

static void dejson_parse_short(dejson_state_t* state, void* data)
{
  *(short*)data = dejson_get_int64(state, SHRT_MIN, SHRT_MAX);
}

static void dejson_parse_ushort(dejson_state_t* state, void* data)
{
  *(unsigned short*)data = dejson_get_uint64(state, USHRT_MAX);
}

static void dejson_parse_int(dejson_state_t* state, void* data)
{
  *(int*)data = dejson_get_int64(state, INT_MIN, INT_MAX);
}

static void dejson_parse_uint(dejson_state_t* state, void* data)
{
  *(unsigned int*)data = dejson_get_uint64(state, UINT_MAX);
}

static void dejson_parse_long(dejson_state_t* state, void* data)
{
  *(long*)data = dejson_get_int64(state, LONG_MIN, LONG_MAX);
}

static void dejson_parse_ulong(dejson_state_t* state, void* data)
{
  *(unsigned long*)data = dejson_get_uint64(state, ULONG_MAX);
}

static void dejson_parse_int8(dejson_state_t* state, void* data)
{
  *(int8_t*)data = dejson_get_int64(state, INT8_MIN, INT8_MAX);
}

static void dejson_parse_int16(dejson_state_t* state, void* data)
{
  *(int16_t*)data = dejson_get_int64(state, INT16_MIN, INT16_MAX);
}

static void dejson_parse_int32(dejson_state_t* state, void* data)
{
  *(int32_t*)data = dejson_get_int64(state, INT32_MIN, INT32_MAX);
}

static void dejson_parse_int64(dejson_state_t* state, void* data)
{
  *(int64_t*)data = dejson_get_int64(state, INT64_MIN, INT64_MAX);
}

static void dejson_parse_uint8(dejson_state_t* state, void* data)
{
  *(uint8_t*)data = dejson_get_uint64(state, UINT8_MAX);
}

static void dejson_parse_uint16(dejson_state_t* state, void* data)
{
  *(uint16_t*)data = dejson_get_uint64(state, UINT16_MAX);
}

static void dejson_parse_uint32(dejson_state_t* state, void* data)
{
  *(uint32_t*)data = dejson_get_uint64(state, UINT32_MAX);
}

static void dejson_parse_uint64(dejson_state_t* state, void* data)
{
  *(uint64_t*)data = dejson_get_uint64(state, UINT64_MAX);
}

static void dejson_parse_float(dejson_state_t* state, void* data)
{
  *(float*)data = dejson_get_double(state, FLT_MIN, FLT_MAX);
}

static void dejson_parse_double(dejson_state_t* state, void* data)
{
  *(double*)data = dejson_get_double(state, DBL_MIN, DBL_MAX);
}

static void dejson_parse_boolean(dejson_state_t* state, void* data)
{
  const uint8_t* json = state->json;

  if (json[0] == 't' && json[1] == 'r' && json[2] == 'u' && json[3] == 'e' && !isalpha(json[4]))
  {
    *(char*)data = 1;
    state->json += 4;
  }
  else if (json[0] == 'f' && json[1] == 'a' && json[2] == 'l' && json[3] == 's' && json[4] == 'e' && !isalpha(json[5]))
  {
    *(char*)data = 0;
    state->json += 5;
  }
  else
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }
}

static void dejson_parse_string(dejson_state_t* state, void* data)
{
  const uint8_t* aux = state->json;
  size_t length = dejson_skip_string(state);
  uint8_t* str = (uint8_t*)dejson_alloc(state, length + 1, DEJSON_ALIGNOF(char));

  if (state->counting)
  {
    return;
  }

  if (*aux++ != '"')
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  ((dejson_string_t*)data)->chars = (char*)str;
  
  if (*aux !='"')
  {
    do
    {
      if (*aux == '\\')
      {
        char digits[5];
        uint32_t utf32;

        aux++;

        switch (*aux++)
        {
        case '"':  *str++ = '"'; break;
        case '\\': *str++ = '\\'; break;
        case '/':  *str++ = '/'; break;
        case 'b':  *str++ = '\b'; break;
        case 'f':  *str++ = '\f'; break;
        case 'n':  *str++ = '\n'; break;
        case 'r':  *str++ = '\r'; break;
        case 't':  *str++ = '\t'; break;
        
        case 'u':
          digits[0] = aux[0];
          digits[1] = aux[1];
          digits[2] = aux[2];
          digits[3] = aux[3];
          digits[4] = 0;
          aux += 4;

          utf32 = strtoul(digits, NULL, 16);

          if (utf32 < 0x80)
          {
            *str++ = utf32;
          }
          else if (utf32 < 0x800)
          {
            str[0] = 0xc0 | (utf32 >> 6);
            str[1] = 0x80 | (utf32 & 0x3f);
            str += 2;
          }
          else if (utf32 < 0x10000)
          {
            str[0] = 0xe0 | (utf32 >> 12);
            str[1] = 0x80 | ((utf32 >> 6) & 0x3f);
            str[2] = 0x80 | (utf32 & 0x3f);
            str += 3;
          }
          else
          {
            str[0] = 0xf0 | (utf32 >> 18);
            str[1] = 0x80 | ((utf32 >> 12) & 0x3f);
            str[2] = 0x80 | ((utf32 >> 6) & 0x3f);
            str[3] = 0x80 | (utf32 & 0x3f);
            str += 4;
          }

          break;
        
        default:
          longjmp(state->rollback, DEJSON_INVALID_ESCAPE);
        }
      }
      else
      {
        *str++ = *aux++;
      }
    }
    while (*aux != '"');
  }

  *str = 0;
}

typedef void (*dejson_parser_t)(dejson_state_t*, void*);

static const dejson_parser_t dejson_parsers[] =
{
  dejson_parse_char, dejson_parse_uchar, dejson_parse_short, dejson_parse_ushort,
  dejson_parse_int, dejson_parse_uint, dejson_parse_long, dejson_parse_ulong,
  dejson_parse_int8, dejson_parse_int16, dejson_parse_int32, dejson_parse_int64,
  dejson_parse_uint8, dejson_parse_uint16, dejson_parse_uint32, dejson_parse_uint64,
  dejson_parse_float, dejson_parse_double, dejson_parse_boolean, dejson_parse_string
};

static void dejson_parse_value(dejson_state_t*, void*, const dejson_record_field_meta_t*);
static void dejson_parse_object(dejson_state_t*, void*, const dejson_record_meta_t*);

static void dejson_parse_array(dejson_state_t* state, void* value, size_t element_size, size_t element_alignment, const dejson_record_field_meta_t* field)
{
  if (*state->json != '[')
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  const uint8_t* save = state->json;
  size_t count = dejson_skip_array(state);
  state->json = save + 1;

  uint8_t* elements = (uint8_t*)dejson_alloc(state, element_size * count, element_alignment);
  
  dejson_array_t* array = (dejson_array_t*)value;

  if (!state->counting)
  {
    array->elements = elements;
    array->count = count;
    array->element_size = element_size;
  }

  dejson_skip_spaces(state);

  dejson_record_field_meta_t field_scalar = *field;
  field_scalar.flags &= ~DEJSON_FLAG_ARRAY;

  while (*state->json != ']')
  {
    dejson_parse_value(state, (void*)elements, &field_scalar);
    dejson_skip_spaces(state);

    elements += element_size;
    
    if (*state->json != ',')
    {
      break;
    }

    state->json++;
    dejson_skip_spaces(state);
  }

  if (*state->json != ']')
  {
    longjmp(state->rollback, DEJSON_UNTERMINATED_ARRAY);
  }

  state->json++;
}

static void dejson_parse_value(dejson_state_t* state, void* value, const dejson_record_field_meta_t* field)
{
#define DEJSON_TYPE_INFO(t) sizeof(t), DEJSON_ALIGNOF(t)
  
  static const size_t dejson_type_info[] =
  {
    DEJSON_TYPE_INFO(char), DEJSON_TYPE_INFO(unsigned char), DEJSON_TYPE_INFO(short), DEJSON_TYPE_INFO(unsigned short),
    DEJSON_TYPE_INFO(int), DEJSON_TYPE_INFO(unsigned int), DEJSON_TYPE_INFO(long), DEJSON_TYPE_INFO(unsigned long),
    DEJSON_TYPE_INFO(int8_t), DEJSON_TYPE_INFO(int16_t), DEJSON_TYPE_INFO(int32_t), DEJSON_TYPE_INFO(int64_t),
    DEJSON_TYPE_INFO(uint8_t), DEJSON_TYPE_INFO(uint16_t), DEJSON_TYPE_INFO(uint32_t), DEJSON_TYPE_INFO(uint64_t),
    DEJSON_TYPE_INFO(float), DEJSON_TYPE_INFO(double), DEJSON_TYPE_INFO(char), DEJSON_TYPE_INFO(dejson_string_t)
  };
  
  const dejson_record_meta_t* meta;

  if ((field->flags & (DEJSON_FLAG_ARRAY | DEJSON_FLAG_POINTER)) == 0)
  {
    if (field->type != DEJSON_TYPE_RECORD)
    {
      char dummy[64];

      if (state->counting)
      {
        value = (void*)dummy;
      }

      dejson_parsers[field->type](state, value);
    }
    else
    {
      meta = dejson_resolve_record(field->type_hash);
      
      if (meta == NULL)
      {
        longjmp(state->rollback, DEJSON_UNKOWN_RECORD);
      }

      dejson_parse_object(state, value, meta);
    }

    return;
  }

  size_t size, alignment;

  if (field->type != DEJSON_TYPE_RECORD)
  {
    unsigned ndx = field->type * 2;
    size = dejson_type_info[ndx];
    alignment = dejson_type_info[ndx + 1];
  }
  else /* field->type == DEJSON_TYPE_RECORD */
  {
    meta = dejson_resolve_record(field->type_hash);
    
    if (meta == NULL)
    {
      longjmp(state->rollback, DEJSON_UNKOWN_RECORD);
    }

    size = meta->size;
    alignment = meta->alignment;
  }

  if ((field->flags & DEJSON_FLAG_ARRAY) != 0)
  {
    dejson_parse_array(state, value, size, alignment, field);
    return;
  }

  // (field->flags & DEJSON_FLAG_POINTER) != 0

  const uint8_t* json = state->json;

  if (json[0] == 'n' && json[1] == 'u' && json[2] == 'l' && json[3] == 'l' && !isalpha(json[4]))
  {
    if (!state->counting)
    {
      *(void**)value = NULL;
    }

    state->json += 4;
    return;
  }

  void* pointer = dejson_alloc(state, size, alignment);

  if (!state->counting)
  {
    *(void**)value = pointer;
  }

  value = pointer;

  if (field->type != DEJSON_TYPE_RECORD)
  {
    dejson_parsers[field->type](state, value);
  }
  else
  {
    dejson_parse_object(state, value, meta);
  }
}

static void dejson_parse_object(dejson_state_t* state, void* record, const dejson_record_meta_t* meta)
{
  if (*state->json != '{')
  {
    longjmp(state->rollback, DEJSON_INVALID_VALUE);
  }

  if (!state->counting)
  {
    memset((void*)record, 0, meta->size);
  }

  state->json++;
  dejson_skip_spaces(state);
  
  while (*state->json != '}')
  {
    if (*state->json != '"')
    {
      longjmp(state->rollback, DEJSON_MISSING_KEY);
    }

    const char* key = (const char*)++state->json;
    const char* quote = key;
    
    for (;;)
    {
      quote = strchr(quote, '"');

      if (!quote)
      {
        longjmp(state->rollback, DEJSON_UNTERMINATED_KEY);
      }

      if (quote[-1] != '\\')
      {
        break;
      }
    }

    state->json = (const uint8_t*)quote + 1;
    uint32_t hash = dejson_hash((const uint8_t*)key, quote - key);

    unsigned i;
    const dejson_record_field_meta_t* field;
    
    for (i = 0, field = meta->fields; i < meta->num_fields; i++, field++)
    {
      if (field->name_hash == hash)
      {
        break;
      }
    }

    dejson_skip_spaces(state);

    if (*state->json != ':')
    {
      longjmp(state->rollback, DEJSON_MISSING_VALUE);
    }

    state->json++;
    dejson_skip_spaces(state);

    if (i != meta->num_fields)
    {
      dejson_parse_value(state, (void*)((uint8_t*)record + field->offset), field);
    }
    else
    {
      dejson_skip_value(state);
    }

    dejson_skip_spaces(state);

    if (*state->json != ',')
    {
      break;
    }

    state->json++;
    dejson_skip_spaces(state);
  }

  if (*state->json != '}')
  {
    longjmp(state->rollback, DEJSON_UNTERMINATED_OBJECT);
  }

  state->json++;
}

static int dejson_execute(void* buffer, uint32_t hash, const uint8_t* json, int counting)
{
  const dejson_record_meta_t* meta = dejson_resolve_record(hash);
  
  if (!meta)
  {
    return DEJSON_UNKOWN_RECORD;
  }

  dejson_state_t state;
  int res;
  
  if ((res = setjmp(state.rollback)) != 0)
  {
    return res;
  }

  state.json = json;
  state.buffer = counting ? 0 : (uintptr_t)buffer;
  state.counting = counting;

  void* record = dejson_alloc(&state, meta->size, meta->alignment);
  
  dejson_skip_spaces(&state);
  dejson_parse_object(&state, record, meta);
  dejson_skip_spaces(&state);

  if (counting)
  {
    *(size_t*)buffer = state.buffer;
  }

  return *state.json == 0 ? DEJSON_OK : DEJSON_EOF_EXPECTED;
}

int dejson_deserialize(void* buffer, uint32_t hash, const uint8_t* json)
{
  return dejson_execute(buffer, hash, json, 0);
}

int dejson_get_size(size_t* size, uint32_t hash, const uint8_t* json)
{
  return dejson_execute((void*)size, hash, json, 1);
}

uint32_t dejson_hash(const uint8_t* str, size_t length)
{
  uint32_t hash = 5381;

  if (length != 0)
  {
    do
    {
      hash = hash * 33 + *str++;
    }
    while (--length != 0);
  }

  return hash;
}
