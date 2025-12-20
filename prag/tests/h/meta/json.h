#pragma once
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include "field.h"
#include "meta.h"

namespace meta
{

// -------------------------
// Arithmetic and string
// -------------------------
template <Arithmetic T> std::string toJson(const T& value)
{
    return std::to_string(value);
}

inline std::string toJson(const std::string& value)
{
    return "\"" + value + "\"";
}

// -------------------------
// Vector
// -------------------------
template <Vector T> std::string toJson(const T& vec)
{
    std::string out = "[";
    bool first = true;
    for (const auto& v : vec)
    {
        if (!first)
            out += ",";
        out += toJson(v);
        first = false;
    }
    out += "]";
    return out;
}

// -------------------------
// Map
// -------------------------
template <Map T> std::string toJson(const T& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : m)
    {
        if (!first)
            out += ",";
        out += toJson(k) + ":" + toJson(v);
        first = false;
    }
    out += "}";
    return out;
}

// -------------------------
// Optional
// -------------------------
template <Optional T> std::string toJson(const T& opt)
{
    if (opt.has_value())
        return toJson(*opt);
    return "null";
}

template <Variant T> std::string toJson(const T& var)
{
    std::string result = "null";
    std::visit([&](const auto& value) { result = toJson(value); }, var);
    return result;
}

// -------------------------
// Object (via MetaTuple)
// -------------------------

template <Object T> std::string toJson(const T& obj)
{
    std::ostringstream ss;
    ss << "{";
    constexpr auto& fields = MetaTuple<T>::fields;
    size_t index = 0;
    std::apply(
        [&](auto&&... flds)
        {
            (...,
             [&](auto& fld)
             {
                 if (index++ > 0)
                     ss << ",";
                 ss << "\"" << fld.fieldName << "\":";
                 using FieldType = std::remove_reference_t<decltype(fld)>;
                 if constexpr (FieldType::memberPtr != nullptr)
                 {
                     ss << toJson(obj.*(FieldType::memberPtr));
                 }
                 else if constexpr (FieldType::getterPtr != nullptr)
                 {
                     ss << toJson(FieldType::getterPtr(obj));
                 }
             }(flds));
        },
        fields);
    ss << "}";
    return ss.str();
}
} // namespace meta
