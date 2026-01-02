#pragma once
#include <sstream>
#include <string>

#include "ast.h"
#include "registry_ast_walker.h"

namespace bhw
{
class FSharpAstWalker : public RegistryAstWalker
{
  public:
    FSharpAstWalker() : RegistryAstWalker(bhw::Language::FSharp)
    {
    }

    Language getLang() override
    {
        return bhw::Language::FSharp;
    }

  protected:
    // Header for the F# file
    std::string generateHeader(const bhw::Ast&) override
    {
        return "namespace Generated\n\nopen System\nopen System.Collections.Generic\n\n";
    }

    // Struct type definition
    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip during flatten pass
        }
        return "type " + s.name + " = {\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip fields during flatten pass
        }

        std::ostringstream out;
        out << ctx.indent() << capitalize(field.name) << ": " << walkType(*field.type, ctx) << "\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return "}\n\n";
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return "type " + e.name + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool last, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << "  | " << val.name << " = " << val.number;
        if (!last)
            out << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return "\n\n";
    }

    std::string generateStructType(const StructType& type, const WalkContext&) override
    {
        return type.value->name;
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext&) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
            return "sbyte";
        case ReifiedTypeId::UInt8:
            return "byte";
        case ReifiedTypeId::Int16:
            return "int16";
        case ReifiedTypeId::UInt16:
            return "uint16";
        case ReifiedTypeId::Int32:
            return "int";
        case ReifiedTypeId::UInt32:
            return "uint32";
        case ReifiedTypeId::Int64:
            return "int64";
        case ReifiedTypeId::UInt64:
            return "uint64";
        case ReifiedTypeId::Float32:
            return "float32";
        case ReifiedTypeId::Float64:
            return "float";
        case ReifiedTypeId::String:
            return "string";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::Bytes:
            return "byte[]";
        default:
            return "obj";
        }
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << walkType(*type.args[0], ctx) << " list";
            break;
        case ReifiedTypeId::Array:
            out << walkType(*type.args[0], ctx) << "[]";
            break;
        case ReifiedTypeId::Set:
            out << "Set<" << walkType(*type.args[0], ctx) << ">";
            break;
        case ReifiedTypeId::Map:
            out << "Map<" << walkType(*type.args[0], ctx) << ", " << walkType(*type.args[1], ctx)
                << ">";
            break;
        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ctx) << " option";
            break;
        case ReifiedTypeId::Variant:
            // F# doesn't have built-in variant types, use discriminated union
            out << "obj"; // Fallback
            break;
        default:
            out << "byte[]";
            break;
        }
        return out.str();
    }

    // Oneof handling - F# uses discriminated unions
    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            // Generate discriminated union during flatten pass
            std::ostringstream out;
            out << "type " << capitalize(oneof.name) << " =\n";

            for (const auto& field : oneof.fields)
            {
                out << "  | " << capitalize(field.name) << " of " << walkType(*field.type, ctx)
                    << "\n";
            }
            out << "\n";
            return out.str();
        }
        else
        {
            // Generate field in record during normal pass
            std::ostringstream out;
            out << ctx.indent() << capitalize(oneof.name) << ": " << capitalize(oneof.name) << "\n";
            return out.str();
        }
    }

    inline std::string capitalize(const std::string& s)
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[0])));
        return r;
    }
};
} // namespace bhw