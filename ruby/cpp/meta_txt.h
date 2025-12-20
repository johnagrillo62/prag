#pragma once
#include <windows.h>

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>
#include <utility>
#include <vector>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <tuple>


inline void removeBlanks(std::string& str)
{
    // Remove all whitespace characters
    std::erase_if(str, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

namespace bhw
{

// ================================================================
// Generic appendField() helpers
// ================================================================
template <typename T>
inline void appendField(std::ostringstream& os, std::string_view name, const T& value)
{
    os << name << "=" << value;
}

// Specialization for bool
inline void appendField(std::ostringstream& os, std::string_view name, bool value)
{
    os << name << "=" << (value ? "true" : "false");
}

// Specialization for time_point
inline void appendField(std::ostringstream& os,
                        std::string_view name,
                        const std::chrono::system_clock::time_point& tp)
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    os << name << "=" << std::put_time(&tm, "%Y-%m-%d");
}

template <typename T> constexpr std::string_view raw_type_name()
{
#if defined(__clang__) || defined(__GNUC__)
    std::string_view p = __PRETTY_FUNCTION__;
    auto start = p.find("T = ") + 4;
    auto end = p.find_first_of(";]", start);
    return p.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view p = __FUNCSIG__;
    auto start = p.find("raw_type_name<") + 15;
    auto end = p.find(">(void)", start);
    return p.substr(start, end - start);
#endif
}

constexpr std::string_view strip_class_prefix(std::string_view name)
{
    if (name.starts_with("class "))
        name.remove_prefix(6);
    else if (name.starts_with("struct "))
        name.remove_prefix(7);
    return name;
}

template <typename T> constexpr std::string_view type_name(bool fullyQualified = false)
{
    auto full = strip_class_prefix(raw_type_name<T>());
    if (fullyQualified)
        return full;

    // Strip namespaces for short name
    auto pos = full.find_last_of(':');
    if (pos != std::string_view::npos)
        return full.substr(pos + 1);
    return full;
}

// ================================================================
// Generic reflect_to_string()
// Works for any class with a `fields` tuple and matching members.
// ================================================================
template <typename T> std::string toString(const T& obj)
{
    std::ostringstream os;
    os << type_name<T>() << " { ";
    std::apply(
        [&](const auto&... field)
        {
            size_t i = 0;
            constexpr size_t N = sizeof...(field);

            ((appendField(os, field.memberName, obj.*(field.memberPtr)),
              os << (++i < N ? ", " : "")),
             ...);
        },
        T::fields);

    os << " }";
    return os.str();
}


} // namespace bhw
