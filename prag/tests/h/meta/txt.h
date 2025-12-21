#pragma once
#include <chrono>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <optional>
#include <vector>
#include <variant>
#include <iostream>
#include <typeinfo>
#include "meta.h"

namespace meta
{

// Prettify type names from __PRETTY_FUNCTION__
inline auto prettifyType(std::string_view typeStr) -> std::string {
    auto result = std::string(typeStr);
    
    // IMPORTANT: Do replacements in order from most specific to least specific!
    
    // 1. Remove the extra type info that GCC adds
    auto pos = size_t{0};
    while ((pos = result.find("; std::string_view = std::basic_string_view<char>")) != std::string::npos) {
        result.erase(pos, 51);
    }
    
    // 2. Replace verbose std::string variations with std::string
    const std::string_view patterns[] = {
        "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >",
        "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>>",
        "std::__cxx11::basic_string<char>",
    };
    
    for (const auto& pattern : patterns) {
        pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.size(), "std::string");
            pos += 11; // Length of "std::string"
        }
    }
    
    // 3. Clean up spacing around angle brackets
    pos = 0;
    while ((pos = result.find(" >")) != std::string::npos) {
        result.erase(pos, 1);
    }
    
    pos = 0;
    while ((pos = result.find("  ")) != std::string::npos) {
        result.replace(pos, 2, " ");
    }
    
    return result;
}
  
// Field value serialization functions
template <Arithmetic T>
auto fieldValueToString(const T& value) -> std::string
{
    auto ss = std::ostringstream{};
    ss << value;
    return ss.str();
}

inline auto fieldValueToString(const std::string& value) -> std::string
{
    return "\"" + value + "\"";
}

template <Optional T>
auto fieldValueToString(const T& opt) -> std::string
{
    if (opt.has_value())
        return fieldValueToString(*opt);
    return "nullopt";
}

template <Vector T>
auto fieldValueToString(const T& vec) -> std::string
{
    auto out = std::string{"["};
    for (auto i = size_t{0}; i < vec.size(); ++i)
    {
        out += fieldValueToString(vec[i]);
        if (i < vec.size() - 1)
            out += ", ";
    }
    out += "]";
    return out;
}

template <Tuple T>
auto fieldValueToString(const T& value) -> std::string
{
    auto out = std::string{"("};
    std::apply([&out](auto&&... elems)
    {
        auto n = size_t{0};
        ((out += fieldValueToString(elems) + (++n < sizeof...(elems) ? ", " : "")), ...);
    }, value);
    out += ")";
    return out;
}

template <Map T>
auto fieldValueToString(const T& m) -> std::string
{
    auto out = std::string{"{"};
    auto first = true;
    for (const auto& [key, val] : m)
    {
        if (!first) out += ", ";
        out += fieldValueToString(key) + ": " + fieldValueToString(val);
        first = false;
    }
    out += "}";
    return out;
}

template <Variant T>
auto fieldValueToString(const T& var) -> std::string
{
    auto result = std::string{"null"};
    std::visit([&](const auto& value) {
        result = fieldValueToString(value);
    }, var);
    return result;
}

// Extract field as string for debug/display
template <typename Instance, typename FieldMeta>
auto extractFieldString(const Instance& instance, const FieldMeta& fieldMeta) -> std::string
{
    auto ss = std::ostringstream{};
    
    // Get type name via __PRETTY_FUNCTION__ and prettify it
    constexpr auto typeStr = FieldMeta::getTypeName();
    
    ss << fieldMeta.fieldName << " (" << prettifyType(typeStr) << "): ";

    // Check if field HAS Props attribute at compile-time
    if constexpr (FieldMeta::template has<Props>())
    {
        // Check the actual value at runtime
        if ((fieldMeta.getProps() & meta::Prop::Private) != meta::Prop::None)
        {
            ss << "<inaccessible - private member>";
            return ss.str();
        }
    }
    
    // Normal field handling
    if constexpr (FieldMeta::memberPtr != nullptr)
    {
        ss << fieldValueToString(instance.*(FieldMeta::memberPtr));
    }
    else if constexpr (FieldMeta::getterPtr != nullptr)
    {
        auto getter = FieldMeta::getterPtr;
        if (getter)
            ss << fieldValueToString(getter(instance));
        else
            ss << "<no getter available>";
    }
    else
    {
        ss << "<no member pointer or getter>";
    }

    return ss.str();
}

// Convert object to string representation
template <Object T>
auto toString(const T& instance) -> std::string
{
    auto ss = std::ostringstream{};
    constexpr auto& fields = meta::MetaTuple<T>::fields;
    std::apply([&](auto&&... fieldMetas)
               { ((ss << extractFieldString(instance, fieldMetas) << "\n"), ...); },
               fields);
    return ss.str();
}

} // namespace meta

