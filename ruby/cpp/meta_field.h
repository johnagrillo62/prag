#pragma once
#include <windows.h>

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace bhw
{
enum FieldProp : uint8_t
{
    None = 0,
    PrimaryKey = 1 << 0,
    Setter = 1 << 1,
    Getter = 1 << 2,
    Serializable = 1 << 3,
    Hashable = 1 << 4
};

// Enable bitwise OR for convenience
constexpr FieldProp operator|(FieldProp a, FieldProp b)
{
    return static_cast<FieldProp>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr FieldProp operator&(FieldProp a, FieldProp b)
{
    return static_cast<FieldProp>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// Field metadata template
template <typename Class, typename T, FieldProp Props> struct FieldMeta
{
    using type = T;
    static constexpr FieldProp properties = Props;
    T Class::* memberPtr;
    const char* memberName;
    const char* sqlColumnName;
    const char* csvColumnName;
};
} // namespace bhw
