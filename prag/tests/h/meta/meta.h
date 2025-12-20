#pragma once
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>
#include <concepts>

namespace
{
template <typename T> struct is_vector : std::false_type
{
};
template <typename T> struct is_vector<std::vector<T>> : std::true_type
{
};
template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T> struct is_tuple : std::false_type
{
};
template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type
{
};
template <typename T> inline constexpr bool is_tuple_v = is_tuple<T>::value;

template <typename T> struct is_map : std::false_type
{
};
template <typename K, typename V> struct is_map<std::map<K, V>> : std::true_type
{
};
template <typename T> inline constexpr bool is_map_v = is_map<T>::value;

template <typename T> struct is_optional : std::false_type
{
};
template <typename U> struct is_optional<std::optional<U>> : std::true_type
{
};
template <typename T> inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T> struct is_string : std::false_type
{
};
template <> struct is_string<std::string> : std::true_type
{
};
template <typename T> inline constexpr bool is_string_v = is_string<T>::value;

template <typename T> struct is_variant : std::false_type
{
};
template <typename... Args> struct is_variant<std::variant<Args...>> : std::true_type
{
};
template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;
}



namespace meta
{

template <typename T>
concept String = std::is_same_v<T, std::string>;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Vector = is_vector_v<T>;

template <typename T>
concept Map = is_map_v<T>;

template <typename T>
concept Optional = is_optional_v<T>;

template <typename T>
concept Variant = is_variant_v<T>;

template <typename T>
concept Object =
    !Arithmetic<T> && !String<T> && !Vector<T> && !Map<T> && !Optional<T> && !Variant<T>;

template <typename T>
concept Tuple = is_tuple_v<T>;

} // namespace meta
