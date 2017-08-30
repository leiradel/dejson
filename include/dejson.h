#ifndef __DEJSON_H__
#define __DEJSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define DEJSON_OFFSETOF(s, f) ((size_t)(&((s*)0)->f))
#define DEJSON_ALIGNOF(t)     DEJSON_OFFSETOF(struct{char c; t d;}, d)

enum
{
  DEJSON_OK,
  DEJSON_OBJECT_EXPECTED,
  DEJSON_UNKOWN_RECORD,
  DEJSON_EOF_EXPECTED,
  DEJSON_MISSING_KEY,
  DEJSON_UNTERMINATED_KEY,
  DEJSON_MISSING_VALUE,
  DEJSON_UNTERMINATED_OBJECT,
  DEJSON_INVALID_VALUE,
  DEJSON_UNTERMINATED_STRING,
  DEJSON_UNTERMINATED_ARRAY,
  DEJSON_INVALID_ESCAPE
};

enum
{
  DEJSON_TYPE_CHAR,
  DEJSON_TYPE_UCHAR,
  DEJSON_TYPE_SHORT,
  DEJSON_TYPE_USHORT,
  DEJSON_TYPE_INT,
  DEJSON_TYPE_UINT,
  DEJSON_TYPE_LONG,
  DEJSON_TYPE_ULONG,
  DEJSON_TYPE_INT8,
  DEJSON_TYPE_INT16,
  DEJSON_TYPE_INT32,
  DEJSON_TYPE_INT64,
  DEJSON_TYPE_UINT8,
  DEJSON_TYPE_UINT16,
  DEJSON_TYPE_UINT32,
  DEJSON_TYPE_UINT64,
  DEJSON_TYPE_FLOAT,
  DEJSON_TYPE_DOUBLE,
  DEJSON_TYPE_BOOL,
  DEJSON_TYPE_STRING,
  DEJSON_TYPE_RECORD
};

enum
{
  DEJSON_FLAG_ARRAY   = 1 << 0,
  DEJSON_FLAG_POINTER = 1 << 1
};

typedef struct
{
  const char* chars;
}
dejson_string_t;

typedef struct
{
  void*    elements;
  uint32_t count;
  uint32_t element_size;
}
dejson_array_t;

#define DEJSON_GET_ELEMENT(array, ndx) \
  ((void*)((uint8_t*)(array).elements + ndx * (array).element_size))

typedef struct
{
  uint32_t name_hash;
  uint32_t type_hash;
  uint32_t offset;
  uint8_t  type;
  uint8_t  flags;
}
dejson_record_field_meta_t;

typedef struct
{
  const dejson_record_field_meta_t* fields;

  uint32_t name_hash;
  uint32_t size;
  uint16_t alignment;
  uint8_t  num_fields;
}
dejson_record_meta_t;

int      dejson_deserialize(void* buffer, uint32_t hash, const uint8_t* json);
int      dejson_get_size(size_t* size, uint32_t hash, const uint8_t* json);
uint32_t dejson_hash(const uint8_t* str, size_t length);

/* User-defined resolver function */
const dejson_record_meta_t* dejson_resolve_record(uint32_t hash);

#ifdef __cplusplus
}
#endif

#endif /* __DEJSON_H__ */
