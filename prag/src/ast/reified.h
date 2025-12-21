#pragma once
// NOTE: This enum is processed by a code generator to produce reified_enum.def
// Keep this as a simple enum class - no explicit values, no complex initialization
// The generator expects: enum class Name : Type { Value1, Value2, ... };

#include "enums.h"

namespace bhw
{
enum class ReifiedTypeId : uint8_t
{
    // Primitives
    Bool,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float32,
    Float64,
    String,
    Bytes,
    Char,
    // Standard
    DateTime,
    Date,
    Time,
    Duration,
    UUID,
    Decimal,
    URL,
    Email,
    // Containers
    List,
    Map,
    Set,
    Tuple,
    Optional,
    Variant,
    Pair,
    Monostate,
    Array,
    UnorderedMap,
    UnorderedSet,
    // Ownership
    PointerType,
    UniquePtr,
    SharedPtr,
    // User-defined / unknown
    StructRefType,
    Unknown
};

#include "reified_enums.def"
} // namespace bhw
