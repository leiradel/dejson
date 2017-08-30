# dejson

Although JSON is based on a subset of the JavaScript language, my uses of it are always tied to a structured underlying data. Because of that, I've written **dejson** as a way to express JSON schema as C structures.

These structures are compiled by the `dejson` compiler, and generate source code that can be added to a C project to deserialize JSON data. The generated source code is just the C structures plus data describing each structure and their fields; the deserializer is completely data-driven.

The deserialization process doesn't error in case the schema and the data don't match. If a field in the schema doesn't have a corresponding key in the data, it's set to 0 (or false or NULL). If a key in the data doesn't have a corresponding field in the schema, it is disregarded.

## Usage

1. Write your JSON schema with the C-like syntax just like the `RetroAchievements.dej` example.
1. Compile it with the `dejson` compiler. Compile the resulting generated code and `src/dejson.c` along with your source code.
1. Call `dejson_get_size` with the JSON data to get the amount of memory needed to deserialize the data.
1. Call `dejson_deserialize` with a buffer with at least the size returned by `dejson_get_size`.
1. Cast the buffer to a pointer to your main structure and access the fields at will.
1. When the desrialized data is not needed anymore, free the buffer.

Because of *dejson*'s current design, you have to define all structures for your JSON schema in the same `.dej` file.
