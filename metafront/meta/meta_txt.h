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
#include <iostream>
#include "meta_field.h"

// Helper for tuple printing
template <typename Tuple, std::size_t... Is>
void print_tuple_elements(const Tuple& t, std::index_sequence<Is...>)
{
    ((std::cout << (Is == 0 ? "" : ", "), print_field_value(std::get<Is>(t))), ...);
}

// Type detection traits
template <typename T> struct is_vector : std::false_type {};
template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T> struct is_tuple : std::false_type {};
template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
template <typename T> inline constexpr bool is_tuple_v = is_tuple<T>::value;

template <typename T> struct is_map : std::false_type {};
template <typename K, typename V> struct is_map<std::map<K, V>> : std::true_type {};
template <typename T> inline constexpr bool is_map_v = is_map<T>::value;

template <typename T> struct is_optional : std::false_type {};
template <typename T> struct is_optional<std::optional<T>> : std::true_type {};
template <typename T> inline constexpr bool is_optional_v = is_optional<T>::value;

// Forward declaration
template <typename T> void print_field_value(const T& value);

template <typename T> void print_field_value(const T& value)
{
    if constexpr (is_optional_v<T>)
    {
        if (value.has_value())
        {
            print_field_value(value.value());
        }
        else
        {
            std::cout << "nullopt";
        }
    }
    else if constexpr (std::is_arithmetic_v<T>)
    {
        std::cout << value;
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        std::cout << "\"" << value << "\"";
    }
    else if constexpr (is_vector_v<T>)
    {
        std::cout << "[";
        for (size_t i = 0; i < value.size(); ++i)
        {
            print_field_value(value[i]);
            if (i < value.size() - 1)
                std::cout << ", ";
        }
        std::cout << "]";
    }
    else if constexpr (is_tuple_v<T>)
    {
        std::cout << "(";
        std::apply(
            [](auto&&... elems)
            {
                size_t n = 0;
                ((print_field_value(elems), std::cout << (++n < sizeof...(elems) ? ", " : "")),
                 ...);
            },
            value);
        std::cout << ")";
    }
    else if constexpr (is_map_v<T>)
    {
        std::cout << "{";
        bool first = true;
        for (const auto& [key, val] : value)
        {
            if (!first)
                std::cout << ", ";
            print_field_value(key);
            std::cout << ": ";
            print_field_value(val);
            first = false;
        }
        std::cout << "}";
    }
    else
    {
        std::cout << "{unknown type: " << typeid(T).name() << "}";
    }
}

template <typename Instance, typename FieldMeta>
void extract_real_field_value(const Instance& instance, const FieldMeta& field_meta)
{
    // Print member name and type
    std::cout << field_meta.fieldName << " (" << field_meta.fieldType << "): ";
    
    // Check if private
    if constexpr ((FieldMeta::properties & meta::Prop::Private) != 0)
    {
        std::cout << "<inaccessible - private member>";
    }
    else
    {
        // Check if member pointer exists
        if constexpr (FieldMeta::memberPtr != nullptr)
        {
            auto actual_value = instance.*(FieldMeta::memberPtr);
            print_field_value(actual_value);
        }
        else
        {
            std::cout << "<no member pointer>";
        }
    }
    std::cout << std::endl;
    
    // Print csv_column and table_column if they exist
    if (field_meta.getCsvColumn().has_value() || field_meta.getSqlColumn().has_value())
    {
        std::cout << "  Attributes: ";
        if (field_meta.getCsvColumn().has_value())
            std::cout << "csv=" << field_meta.getCsvColumn().value() << " ";
        if (field_meta.getSqlColumn().has_value())
	  std::cout << "table=" << field_meta.getSqlColumn().value();
        std::cout << std::endl;
    }
}

template <typename T> void toString(const T& instance)
{
    constexpr auto& fields = meta::MetaTuple<T>::fields;
    std::apply([&instance](auto&&... field_metas)
               { ((extract_real_field_value(instance, field_metas)), ...); },
               fields);
}
