#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "reified.h"

namespace bhw
{

template <typename> inline constexpr bool always_false_v = false;

std::string readFile(const std::string& filename);
std::string getFileExtension(const std::string& filename);
std::string toUpper(std::string& s);
std::string toLower(std::string& s);

struct Type;
struct Field;
struct Struct;
struct Enum;
struct Oneof;
struct Service;

// ---------------- Simple / Ref / Generic / Pointer / Struct Types ----------------
struct SimpleType
{
    std::string srcTypeString;
    ReifiedTypeId reifiedType{};
};

struct StructRefType
{
    std::string srcTypeString;
    ReifiedTypeId reifiedType{};
};

struct PointerType
{
    std::unique_ptr<Type> pointee;
    ReifiedTypeId reifiedType{};
};

struct GenericType
{
    std::vector<std::unique_ptr<Type>> args;
    ReifiedTypeId reifiedType{};
};

struct StructType
{
    std::unique_ptr<Struct> value;
    ReifiedTypeId reifiedType{};
};

struct Attribute
{
    std::string name;
    std::string value;
};

using AttributeVec = std::vector<Attribute>;

// ---------------- Type container ----------------
template <typename T>
concept VariantType = std::same_as<std::remove_cvref_t<T>, SimpleType> ||
                      std::same_as<std::remove_cvref_t<T>, StructRefType> ||
                      std::same_as<std::remove_cvref_t<T>, PointerType> ||
                      std::same_as<std::remove_cvref_t<T>, GenericType> ||
                      std::same_as<std::remove_cvref_t<T>, StructType>;

struct Type
{
    template <VariantType T> explicit Type(T&& t) : value(std::forward<T>(t))
    {
        using BaseT = std::remove_cvref_t<T>;
        if constexpr (std::same_as<BaseT, SimpleType>)
            reifiedTypeId = std::get<SimpleType>(value).reifiedType;
        else if constexpr (std::same_as<BaseT, StructRefType>)
            reifiedTypeId = std::get<StructRefType>(value).reifiedType;
        else if constexpr (std::same_as<BaseT, GenericType>)
            reifiedTypeId = std::get<GenericType>(value).reifiedType;
        else if constexpr (std::same_as<BaseT, PointerType>)
            reifiedTypeId = std::get<PointerType>(value).reifiedType;
        else if constexpr (std::same_as<BaseT, StructType>)
            reifiedTypeId = std::get<StructType>(value).reifiedType;
        else
            static_assert(always_false_v<T>, "Unhandled type !");
    }

    [[nodiscard]] bool isSimple() const
    {
        return std::holds_alternative<SimpleType>(value);
    }
    [[nodiscard]] bool isStructRef() const
    {
        return std::holds_alternative<StructRefType>(value);
    }
    [[nodiscard]] bool isPointer() const
    {
        return std::holds_alternative<PointerType>(value);
    }
    [[nodiscard]] bool isGeneric() const
    {
        return std::holds_alternative<GenericType>(value);
    }
    [[nodiscard]] bool isStruct() const
    {
        return std::holds_alternative<StructType>(value);
    }

    std::variant<SimpleType, StructRefType, PointerType, GenericType, StructType> value;
    ReifiedTypeId reifiedTypeId{};
    std::string srcType{};
};

// ---------------- Enum ----------------
struct EnumValue
{
    std::string name;
    int number{};
    std::vector<Attribute> attributes;
    std::unique_ptr<Type> type;
};

struct Enum
{
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<EnumValue> values;
    std::vector<Attribute> attributes;
    bool scoped{false};
    std::string underlying_type;

    std::string getFullyQualifiedName() const
    {
        std::string result;
        for (const auto& ns : namespaces)
            result += ns + "::";
        result += name;
        return result;
    }
};

// ---------------- Oneof ----------------
struct OneofField
{
    std::string name;
    std::unique_ptr<Type> type;
    std::vector<Attribute> attributes;
};

struct Oneof
{
    std::string name;
    std::vector<OneofField> fields;
    std::vector<Attribute> attributes;
};

// ---------------- RPC / Service ----------------
struct RpcMethod
{
    std::string name;
    std::string request_type;
    std::string response_type;
    bool client_streaming = false;
    bool server_streaming = false;
    std::vector<Attribute> attributes;
};

struct Service
{
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<RpcMethod> methods;
    std::vector<Attribute> attributes;

    std::string getFullyQualifiedName() const
    {
        std::string result;
        for (const auto& ns : namespaces)
            result += ns + "::";
        result += name;
        return result;
    }
};

// ---------------- Field / StructMember / Struct ----------------
struct Field
{
    std::string name{};
    std::unique_ptr<Type> type{};
    AttributeVec attributes{};
};

using StructMember = std::variant<Field, Oneof, Enum, Struct>;

struct Struct
{
    std::string name;
    std::vector<std::string> namespaces;
    std::vector<StructMember> members;
    std::vector<Attribute> attributes;

    std::string variableName{}; // holds the variable name
    bool isAnonymous = false;   // for C++ anonymous structs

    // C# record variant support
    bool isRecord = false;      // true for C# records, false for classes
    bool isAbstract = false;    // true for abstract modifier
    std::string baseType;       // immediate parent type name (empty if no parent)

    std::string getFullyQualifiedName() const
    {
        std::string result;
        for (const auto& ns : namespaces)
            result += ns + "::";
        result += name;
        return result;
    }
};

// ---------------- Namespace / AST ----------------
struct Namespace;
using AstRootNode = std::variant<Enum, Struct, Namespace, Service, Oneof>;

struct Namespace
{
    std::string name;
    std::vector<AstRootNode> nodes;
    std::vector<Attribute> attributes;

    std::string getFullyQualifiedName() const
    {
        return name;
    }
};

struct Ast
{
    std::string srcName;
    std::vector<std::string> namespaces;
    std::vector<AstRootNode> nodes;

    std::string showAst(size_t indent = 0) const;

    void flattenNestedTypes();
    void flattenStructMembers(Struct& s,
                              std::vector<Struct>& flattenedStructs,
                              std::vector<Enum>& flattenedEnums);
};

// ---------------- Utility ----------------
std::string showType(const Type& type, size_t indent = 0);
std::string showField(const Field& field, size_t indent = 0);
std::string showStruct(const Struct& str, size_t indent = 0);

auto operator<<(std::ostream& os, const SimpleType& s) -> std::ostream&;
auto operator<<(std::ostream& os, const StructRefType& s) -> std::ostream&;

} // namespace bhw
