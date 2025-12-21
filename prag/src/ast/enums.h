#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

namespace bhw
{

// ==================== Enum traits with mapping ====================
template <typename EnumT, auto& MappingArray> struct EnumTraitsAuto
{
    inline static constexpr auto& mapping = MappingArray;

    inline static const std::unordered_map<EnumT, std::string> enumToString = []
    {
        std::unordered_map<EnumT, std::string> m;
        for (auto [e, s] : mapping)
            m[e] = s;
        return m;
    }();

    inline static const std::unordered_map<std::string, EnumT> stringToEnum = []
    {
        std::unordered_map<std::string, EnumT> m;
        for (auto [e, s] : mapping)
            m[s] = e;
        return m;
    }();

    static std::string toString(EnumT e)
    {
        auto it = enumToString.find(e);
        return it != enumToString.end() ? it->second : "";
    }

    static std::optional<EnumT> fromString(const std::string& s)
    {
        auto it = stringToEnum.find(s);
        return it != stringToEnum.end() ? std::optional(it->second) : std::nullopt;
    }

    template <typename Func> static void forEach(Func f)
    {
        for (auto [e, _] : mapping)
            f(e);
    }
};

// ==================== Enum registration ====================
template <typename EnumT> struct EnumMapping; // specialization per enum

// ==================== Concept for registered enums ====================
template <typename T>
concept RegisteredEnum = requires { typename EnumMapping<T>::Type; };

template <RegisteredEnum EnumT> std::ostream& operator<<(std::ostream& os, EnumT e)
{
    using Traits = typename bhw::EnumMapping<EnumT>::Type;
    os << Traits::toString(e);
    return os;
}

// Convert string to enum safely
template <typename EnumT> std::optional<EnumT> to_enum(const std::string& s)
{
    return EnumMapping<EnumT>::Type::fromString(s);
}

// Convert string to enum and throw if invalid
template <typename EnumT> EnumT to_enum_checked(const std::string& s)
{
    auto e = EnumMapping<EnumT>::Type::fromString(s);
    if (!e)
        throw std::runtime_error("Invalid enum string: " + s);
    return *e;
}

} // namespace bhw
