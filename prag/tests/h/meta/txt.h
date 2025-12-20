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
#include "field.h"

namespace meta
{

template <Arithmetic T>
std::string field_value_to_string(const T& value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

std::string field_value_to_string(const std::string& value)
{
    return "\"" + value + "\"";
}

template <Optional T>
std::string field_value_to_string(const T& opt)
{
    if (opt.has_value())
        return field_value_to_string(*opt);
    return "nullopt";
}

template <Vector T>
std::string field_value_to_string(const T& vec)
{
    std::string out = "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        out += field_value_to_string(vec[i]);
        if (i < vec.size() - 1)
            out += ", ";
    }
    out += "]";
    return out;
}

template <Tuple T>
std::string field_value_to_string(const T& value)
{
    std::string out = "(";
    std::apply([&out](auto&&... elems)
    {
        size_t n = 0;
        ((out += field_value_to_string(elems) + (++n < sizeof...(elems) ? ", " : "")), ...);
    }, value);
    out += ")";
    return out;
}

template <Map T>
std::string field_value_to_string(const T& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [key, val] : m)
    {
        if (!first) out += ", ";
        out += field_value_to_string(key) + ": " + field_value_to_string(val);
        first = false;
    }
    out += "}";
    return out;
}

template <Variant T>
std::string field_value_to_string(const T& var)
{
    std::string result = "null";
    std::visit([&](const auto& value) {
        result = field_value_to_string(value);
    }, var);
    return result;
}


template <typename Instance, typename FieldMeta>
std::string extract_field_string(const Instance& instance, const FieldMeta& field_meta)
{
    std::ostringstream ss;
    ss << field_meta.fieldName << " (" << field_meta.fieldType << "): ";

    if constexpr ((FieldMeta::properties & meta::Prop::Private) != meta::Prop::None)
    {
        ss << "<inaccessible - private member>";
    }
    else
    {
        if constexpr (FieldMeta::memberPtr != nullptr)
        {
            ss << field_value_to_string(instance.*(FieldMeta::memberPtr));
        }
        else if constexpr (FieldMeta::getterPtr != nullptr)
        {
            auto getter = FieldMeta::getterPtr;
            if (getter)
                ss << field_value_to_string(getter(instance));
            else
                ss << "<no getter available>";
        }
        else
        {
            ss << "<no member pointer or getter>";
        }
    }

    if (field_meta.getCsvColumn().has_value() || field_meta.getSqlColumn().has_value())
    {
        ss << "  Attributes: ";
        if (field_meta.getCsvColumn().has_value())
            ss << "csv=" << field_meta.getCsvColumn().value() << " ";
        if (field_meta.getSqlColumn().has_value())
            ss << "table=" << field_meta.getSqlColumn().value();
    }

    return ss.str();
}

template <Object T>
std::string toString(const T& instance)
{
    std::ostringstream ss;
    constexpr auto& fields = meta::MetaTuple<T>::fields;
    std::apply([&](auto&&... field_metas)
               { ((ss << extract_field_string(instance, field_metas) << "\n"), ...); },
               fields);
    return ss.str();
}

}

