// zig_walker.h - Zig code generator using AST walker
#pragma once
#include <algorithm>

#include "registry_ast_walker.h"

namespace bhw
{
class ZigAstWalker : public RegistryAstWalker
{
  public:
    ZigAstWalker() : RegistryAstWalker(bhw::Language::Zig)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Zig;
    }
    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "const std = @import(\"std\");\n\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "pub const " << s.name << " = struct {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "};\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << field.name << ": " << walkType(*field.type, ind) << ",\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "pub const " << e.name << " = enum(i32) {\n";
        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;
        std::string zigName = val.name;
        std::transform(zigName.begin(),
                       zigName.end(),
                       zigName.begin(),
                       [](char c) -> char
                       { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
        out << indent(ind) << zigName << " = " << val.number << ",\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t ind) override
    {
        return indent(ind) + "};\n\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, size_t ind) override
    {
        return indent(ind) + "pub const " + ns.name + " = struct {\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, size_t ind) override
    {
        return indent(ind) + "};\n\n";
    }

    std::string generatePointerType(const PointerType& type, size_t ind = 0) override
    {
        std::string result = "*" + walkType(*type.pointee, ind);
        return result;
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return type.value->name;
    }

    std::string generateGenericType(const GenericType& type, size_t ind = 0) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
        case ReifiedTypeId::Set:
            out << "std.ArrayList(" << walkType(*type.args[0], ind) << ")";
            break;

        case ReifiedTypeId::Map:
            out << "std.AutoHashMap(" << walkType(*type.args[0], ind) << ", "
                << walkType(*type.args[1], ind) << ")";
            break;

        case ReifiedTypeId::Optional:
            out << "?" << walkType(*type.args[0], ind);
            break;

        case ReifiedTypeId::Variant:
        {
            out << "union(enum) {\n";
            for (size_t i = 0; i < type.args.size(); ++i)
            {
                std::string typeName = walkType(*type.args[i], ind);
                std::string fieldName = zigFieldName(type.args[i]->reifiedTypeId);
                out << indent(ind + 2) << fieldName << ": " << typeName << ",\n";
            }
            out << indent(ind + 1) << "}";
            break;
        }

        default:
            out << "[]u8";
            break;
        }

        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, size_t) override
    {
        return canonicalToZig(type.reifiedType);
    }

    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "pub const " << oneof.name << " = union(enum) {\n";

        for (const auto& field : oneof.fields)
        {
            out << indent(ind + 1) << field.name << ": " << walkType(*field.type, ind) << ",\n";
        }

        out << indent(ind) << "};\n\n";
        return out.str();
    }

  private:
    std::string canonicalToZig(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
            return "i8";
        case ReifiedTypeId::Int16:
            return "i16";
        case ReifiedTypeId::Int32:
            return "i32";
        case ReifiedTypeId::Int64:
            return "i64";
        case ReifiedTypeId::UInt8:
            return "u8";
        case ReifiedTypeId::UInt16:
            return "u16";
        case ReifiedTypeId::UInt32:
            return "u32";
        case ReifiedTypeId::UInt64:
            return "u64";
        case ReifiedTypeId::Float32:
            return "f32";
        case ReifiedTypeId::Float64:
            return "f64";
        case ReifiedTypeId::String:
            return "[]const u8";
        case ReifiedTypeId::Char:
            return "u8";
        case ReifiedTypeId::Bytes:
            return "[]u8";
        default:
            return "[]u8";
        }
    }

    std::string zigFieldName(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "bool_val";
        case ReifiedTypeId::Int8:
            return "i8_val";
        case ReifiedTypeId::Int16:
            return "i16_val";
        case ReifiedTypeId::Int32:
            return "i32_val";
        case ReifiedTypeId::Int64:
            return "i64_val";
        case ReifiedTypeId::UInt8:
            return "u8_val";
        case ReifiedTypeId::UInt16:
            return "u16_val";
        case ReifiedTypeId::UInt32:
            return "u32_val";
        case ReifiedTypeId::UInt64:
            return "u64_val";
        case ReifiedTypeId::Float32:
            return "f32_val";
        case ReifiedTypeId::Float64:
            return "f64_val";
        case ReifiedTypeId::String:
            return "string_val";
        case ReifiedTypeId::Bytes:
            return "bytes_val";
        default:
            return "val";
        }
    }
};
} // namespace bhw
