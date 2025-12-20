// NOTE: This enum is processed by a code generator to produce languages_enum.def
// Keep this as a simple enum class - no explicit values, no complex initialization
// The generator expects: enum class Name : Type { Value1, Value2, ... };
#pragma once
#include <map>
#include <string>

#include "ast.h"
#include "enums.h"

namespace bhw
{

enum class Language : uint8_t
{
    Avro,
    CSharp, 
    Capnp,
    Cpp26,
    FSharp, 
    FlatBuf,
    Go,
    GraphQl,
    Haskell, 
    JSONSchema,
    Java,
    MDB,
    OCaml, 
    OpenApi,
    ProtoBuf,
    Python,
    Rust,
    Thrift,
    Typescript,
    Zig,
};

#include "languages_enums.def"


} // namespace bhw
