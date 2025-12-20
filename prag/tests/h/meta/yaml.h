#pragma once
#include <sstream>
#include <string>
#include <tuple>

#include "field.h"
#include "meta.h"

namespace meta
{

inline std::string indentStr(int indent)
{
    return std::string(indent * 2, ' ');
}

template <Optional T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    if (value.has_value())
    {
        printYamlField(ss, value.value(), indent);
    }
    else
    {
        ss << "null\n";
    }
}

inline void printYamlField(std::ostringstream& ss, const std::string& value, int indent = 0)
{
    ss << "\"" << value << "\"\n";
}

template <Arithmetic T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    ss << value << "\n";
}

template <Vector T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    for (auto& elem : value)
    {
        ss << indentStr(indent) << "- ";
        printYamlField(ss, elem, indent + 1);
    }
}

template <Tuple T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    std::apply([&](auto&&... elems)
               { ((ss << indentStr(indent), printYamlField(ss, elems, indent)), ...); },
               value);
}

template <Map T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    for (auto& [k, v] : value)
    {
        ss << indentStr(indent) << k << ": ";
        printYamlField(ss, v, indent + 1);
    }
}

template <Variant T> void printYamlField(std::ostringstream& ss, const T& value, int indent = 0)
{
    std::visit([&](const auto& v) { printYamlField(ss, v, indent); }, value);
}

template <typename Instance, typename FieldMeta>
void extractYamlField(std::ostringstream& ss,
                      const Instance& instance,
                      const FieldMeta& fieldMeta,
                      int indent)
{
    ss << indentStr(indent) << fieldMeta.fieldName << ": ";

    if constexpr ((FieldMeta::properties & Prop::Private) != Prop::None)
    {
        ss << "<private>\n";
    }
    else
    {
        if constexpr (FieldMeta::memberPtr != nullptr)
        {
            printYamlField(ss, instance.*(FieldMeta::memberPtr), indent + 1);
        }
        else if constexpr (FieldMeta::getterPtr != nullptr)
        {
            printYamlField(ss, FieldMeta::getterPtr(instance), indent + 1);
        }
        else
        {
            ss << "<no value>\n";
        }
    }
}

// Main toYaml function
template <Object T> std::string toYaml(const T& instance)
{
    std::ostringstream ss;
    constexpr auto& fields = MetaTuple<T>::fields;
    std::apply([&](auto&&... fieldMetas)
               { ((extractYamlField(ss, instance, fieldMetas, 0)), ...); },
               fields);
    return ss.str();
}

} // Namespace meta
