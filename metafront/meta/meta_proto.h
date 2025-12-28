#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

// Map C++ types to proto types
template <typename T> struct ProtoType
{
    static constexpr const char* name = "unknown";
};
template <> struct ProtoType<uint64_t>
{
    static constexpr const char* name = "uint64";
};
template <> struct ProtoType<uint32_t>
{
    static constexpr const char* name = "uint32";
};
template <> struct ProtoType<uint16_t>
{
    static constexpr const char* name = "uint32";
};
template <> struct ProtoType<int32_t>
{
    static constexpr const char* name = "int32";
};
template <> struct ProtoType<int64_t>
{
    static constexpr const char* name = "int64";
};
template <> struct ProtoType<bool>
{
    static constexpr const char* name = "bool";
};
template <> struct ProtoType<std::string>
{
    static constexpr const char* name = "string";
};
template <> struct ProtoType<float>
{
    static constexpr const char* name = "float";
};
template <> struct ProtoType<double>
{
    static constexpr const char* name = "double";
};
// template <> struct ProtoType<ImmutableString> { static constexpr const char* name = "string"; };
template <> struct ProtoType<std::chrono::system_clock::time_point>
{
    static constexpr const char* name = "int64";
};

// Generate proto message from MetaMappable type
template <typename T, std::size_t... I> std::string GenerateProtoImpl(std::index_sequence<I...>)
{
    std::ostringstream os;
    os << "message " << T::table << " {\n";

    // Fold expression over all fields
    ((os << "  optional "
         << ProtoType<typename std::tuple_element<I, decltype(T::fields)>::type::type>::name << " "
         << std::get<I>(T::fields).memberName // <-- pick the correct FieldMeta member
         << " = " << (I + 1) << ";\n"),
     ...);

    os << "}\n";
    return os.str();
}

template <typename T> std::string GenerateProto()
{
    return GenerateProtoImpl<T>(std::make_index_sequence<std::tuple_size_v<decltype(T::fields)>>{});
}
