local ddlt = require 'ddlt'

local parse
parse = function(file)
  local nativeTypes = {
    float = true,
    double = true,
    int = true,
    unsigned = true,
    char = true,
    long = true,
    int8 = true,
    int16 = true,
    int32 = true,
    int64 = true,
    uint8 = true,
    uint16 = true,
    uint32 = true,
    uint64 = true,
    bool = true,
    string = true
  }

  local parser = {
    new = function(self, file)
      self.file = file

      local input, err = io.open(self.file)

      if not input then
        self:error(0, err)
      end

      local source = input:read('*a')
      input:close()

      self.lexer = ddlt.newLexer{
        source = source,
        file = file,
        language = 'cpp',
        symbols = {'{', '}', '[', ']', '<', '>', '*', ';'},
        keywords = {
          'struct', 'signed', 'unsigned', 'char', 'short', 'int', 'long',
          'int8_t', 'int16_t', 'int32_t', 'int64_t', 'uint8_t', 'uint16_t',
          'uint32_t', 'uint64_t', 'float', 'double', 'bool', 'string'
        }
      }

      self.ast = {}
      self.la = {}
      self:next()
    end,

    error = function(self, line, ...)
      local args = {...}
      io.stderr:write(string.format('%s:%d: %s\n', self.file, line, table.concat(args, '')))
      os.exit(1)
    end,

    next = function(self)
      repeat
        local la, err = self.lexer:next(self.la)

        if not la then
          self:error(self.la.line, err)
        end
      until la.token ~= '<linecomment>' and la.token ~= '<blockcomment>'
    end,

    match = function(self, token)
      if token and token ~= self.la.token then
        self:error(self.la.line, token, ' expected, ', self.la.token, ' found')
      end

      self:next()
    end,

    isNativeType = function(self, type)
      return nativeTypes[type]
    end,

    parse = function(self)
      local aggregates = self:parseAggregates()
      self:match('<eof>')

      return aggregates
    end,

    parseAggregates = function(self)
      local aggregates = {}
      local ids = {}

      while self.la.token == 'struct' do
        local struct = self:parseStruct()

        if ids[struct.id] then
          self:error(struct.line, 'duplicated aggregate: ', struct.id)
        end

        aggregates[#aggregates + 1] = struct
        ids[struct.id] = struct
      end

      return aggregates
    end,

    parseStruct = function(self)
      local struct = {fields = {}}
      local ids = {}

      self:match('struct')
      struct.id = self.la.lexeme
      struct.line = self.la.line
      self:match('<id>')
      self:match('{')

      while true do
        local field = self:parseStructField()

        if ids[field.id] then
          self:error(field.line, 'duplicated field: ', field.id)
        end

        struct.fields[#struct.fields + 1] = field
        ids[field.id] = field

        if self.la.token == '}' then
          break
        end
      end

      self:match('}')
      self:match(';')
      return struct
    end,

    parseType = function(self)
      local type = {
        isSigned = false,
        isUnsigned = false,
        isScalar = true,
        isArray = false,
        isPointer = false
      }

      if self.la.token == 'signed' then
        type.isSigned = true
        self:match()
      elseif self.la.token == 'unsigned' then
        type.isUnsigned = true
        self:match()
      end

      if self.la.token == 'signed' then
        if type.isSigned then
          self:error(self.la.line, 'duplicate "signed"')
        elseif type.isUnsigned then
          self:error(self.la.line, '"signed" and "unsigned" specified together')
        end
      elseif self.la.token == 'unsigned' then
        if type.isUnsigned then
          self:error(self.la.line, 'duplicate "unsigned"')
        elseif type.isSigned then
          self:error(self.la.line, '"signed" and "unsigned" specified together')
        end
      end

      type.id = self.la.lexeme
      local t = self.la.token

      if t == 'char' or t == 'short' or t == 'int' or t == 'long' then
        self:match()
      elseif t == 'int8_t' or t == 'int16_t' or t == 'int32_t' or
             t == 'int64_t' or t == 'uint8_t' or t == 'uint16_t' or
             t == 'uint32_t' or t == 'uint64_t' or t == 'float' or
             t == 'double' or t == 'bool' or t == 'string' then
        if type.isSigned or type.isUnsigned then
          self:error(self.la.line, '"signed" or "unsigned" invalid with "', self.la.lexeme, '"')
        end

        self:match()
      elseif t == '<id>' then
        if type.isSigned or type.isUnsigned then
          type.token = 'int'
          type.id = 'int'
        else
          self:match()
        end
      else
        if type.isSigned or type.isUnsigned then
          type.token = 'int'
          type.id = 'int'
        else
          self:error(self.la.line, 'type or identifier expected')
        end
      end

      if self.la.token == '*' then
        type.isPointer = true
        self:match()
      end

      return type
    end,

    parseStructField = function(self)
      local field = {}

      field.type = self:parseType()

      field.id = self.la.lexeme
      field.line = self.la.line
      self:match('<id>')

      if self.la.token == '[' then
        if field.type.isScalar then
          self:match()
          self:match(']')

          field.type.isArray = true
        else
          self:error(self.la.line, 'arrays of pointers are not supported')
        end
      end

      self:match(';')
      return field
    end
  }

  parser:new(file)
  local ast = parser:parse()

  local hash = function(str)
    local h = 5381

    for i = 1, #str do
      h = h * 33 + str:byte(i, i)
      h = bit32.band(h, 0xffffffff)
    end

    return h
  end

  local size = {
    -- 8 bytes
    double = 1,
    int64_t = 1,
    uint64_t = 1,
    -- 8 or 4 bytes
    string = 2,
    long = 2,
    -- 4 bytes
    float = 3,
    int = 3,
    int32_t = 3,
    uint32_t = 3,
    -- 2 bytes
    short = 4,
    int16_t = 4,
    uint16_t = 4,
    -- 1 byte
    char = 5,
    bool = 5,
    int8_t = 5,
    uint8_t = 5
  }

  local unsigned = {
    char = 'DEJSON_TYPE_UCHAR',
    short = 'DEJSON_TYPE_USHORT',
    int = 'DEJSON_TYPE_UINT',
    long = 'DEJSON_TYPE_ULONG'
  }

  local signed = {
    char = 'DEJSON_TYPE_CHAR',
    short = 'DEJSON_TYPE_SHORT',
    int = 'DEJSON_TYPE_INT',
    long = 'DEJSON_TYPE_LONG',
    int8_t = 'DEJSON_TYPE_INT8',
    int16_t = 'DEJSON_TYPE_INT16',
    int32_t = 'DEJSON_TYPE_INT32',
    int64_t = 'DEJSON_TYPE_INT64',
    uint8_y = 'DEJSON_TYPE_UINT8',
    uint16_t = 'DEJSON_TYPE_UINT16',
    uint32_t = 'DEJSON_TYPE_UINT32',
    uint64_t = 'DEJSON_TYPE_UINT64',
    float = 'DEJSON_TYPE_FLOAT',
    double = 'DEJSON_TYPE_DOUBLE',
    bool = 'DEJSON_TYPE_BOOL',
    string = 'DEJSON_TYPE_STRING'
  }

  for i = 1, #ast do
    ast[i].hash = hash(ast[i].id)

    table.sort(ast[i].fields, function(f1, f2)
      local s1 = f1.type.isPointer and 2 or size[f1.type.id] or 2
      local s2 = f2.type.isPointer and 2 or size[f2.type.id] or 2
      return s1 < s2
    end)

    for j = 1, #ast[i].fields do
      local field = ast[i].fields[j]
      local t = field.type
      local sig = ''
      
      field.hash = hash(field.id)
      field.type.hash = hash(t.id)

      if t.isSigned then
        sig = 'signed '
      elseif t.isUnsigned then
        sig = 'unsigned '
      end

      local type = t.id

      if type == 'string' then
        type = 'dejson_string_t'
      elseif type == 'bool' then
        type = 'char'
      end

      if t.isArray then
        field.decl = string.format('dejson_array_t %s;', field.id)
      else
        field.decl = string.format('%s%s%s %s;', sig, type, t.isPointer and '*' or '', field.id)
      end

      if t.isUnsigned then
        field.dejson = unsigned[t.id]
      else
        field.dejson = signed[t.id] or 'DEJSON_TYPE_RECORD'
      end
    end
  end

  return ast
end

local header = [[
#ifndef /*= args.guard */
#define /*= args.guard */

#include <dejson.h>
#include <stdint.h>

/*! for _, aggregate in ipairs(args.ast) do */
typedef struct {
/*!   for _, field in ipairs(aggregate.fields) do */
  /*= field.decl */
/*!   end */
}
/*= aggregate.id */;

extern const dejson_record_meta_t g_Meta/*= aggregate.id */;
/*! end */

const dejson_record_meta_t* dejson_resolve_record(uint32_t hash);

#endif /* /*= args.guard */ */
]]

local code = [[
#include "/*= args.include */"

/*! for _, aggregate in ipairs(args.ast) do */
static const dejson_record_field_meta_t s_fieldMeta/*= aggregate.id */[] = {
/*!   for _, field in ipairs(aggregate.fields) do */
  { /* /*= field.decl */ */
    /* name_hash */ /*= string.format('0x%08xU', field.hash) */,
    /* type_hash */ /*= string.format('0x%08xU', field.dejson == 'DEJSON_TYPE_RECORD' and field.type.hash or 0) */,
    /* offset    */ DEJSON_OFFSETOF(/*= aggregate.id */, /*= field.id */),
    /* type      */ /*= field.dejson */,
    /* flags     */ /*= field.type.isArray and 'DEJSON_FLAG_ARRAY' or (field.type.isPointer and 'DEJSON_FLAG_POINTER' or 0) */
  },
/*!   end */
};

const dejson_record_meta_t g_Meta/*= aggregate.id */ = {
  /* fields     */ s_fieldMeta/*= aggregate.id */,
  /* name_hash  */ /*= string.format('0x%08xU', aggregate.hash) */,
  /* size       */ sizeof(/*= aggregate.id */),
  /* alignment  */ DEJSON_ALIGNOF(/*= aggregate.id */),
  /* num_fields */ /*= #aggregate.fields */
};
/*! end */

const dejson_record_meta_t* dejson_resolve_record(uint32_t hash) {
  switch (hash) {
/*! for _, aggregate in ipairs(args.ast) do */
    case /*= string.format('0x%08xU', aggregate.hash) */: return &g_Meta/*= aggregate.id */;
/*! end */
    default: return NULL;
  }
}
]]

local function generate(options, template, out)
  template = assert(ddlt.newTemplate(template, '/*', '*/'))
  local res = {}
  template(options, function(out) res[#res + 1] = out end)
  res = table.concat(res):gsub('\n+', '\n')

  local file = assert(io.open(out, 'w'))
  file:write(res)
  file:close()
end

local function main(args)
  local genc = false
  local genh = false
  local inputs = {}

  for i = 1, #args do
    if args[i] == '-c' then
      genc = true
    elseif args[i] == '-h' then
      genh = true
    else
      inputs[#inputs + 1] = args[i]
    end
  end

  if #inputs == 0 then
    error('missing input file\n')
  end

  if not (genh or genc) then
    error('nothing to generate')
  end

  for i = 1, #inputs do
    local ast = parse(inputs[i])
    local _, name, ext = ddlt.split(inputs[i])

    local options = {
      ast = ast,
      include = ddlt.join(nil, name, 'h'),
      file = ddlt.realpath(inputs[i]),
      guard = '__' .. ddlt.join(nil, name, 'h'):gsub('[^%w%d]', '_'):upper() .. '__'
    }

    if genh then
      generate(options, header, options.include)
    end

    if genc then
      generate(options, code, ddlt.join(nil, name, 'c'))
    end
  end
end

main(arg)
