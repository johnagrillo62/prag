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
            return "type " + s.name + " = {\n"; // first pass emits full type
        else
            return s.name; // second pass: just the type name
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;
        if (ctx.pass == WalkContext::Pass::Flatten)
            out << "  " << capitalize(field.name) << ": " << walkType(*field.type, ctx) << "\n";
        else
            out << capitalize(field.name) << ": " << walkType(*field.type, ctx)
                << "\n"; // type reference
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "}\n\n"; // close struct in first pass
        else
            return ""; // nothing in second pass
    }

    std::string generateStructType(const StructType& type, const WalkContext&) override
    {
        // always just the type name; first pass already emitted full definition
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
        {
            static int counter = 0;
            if (ctx.pass == WalkContext::Pass::Flatten)
                out << "Variant" << counter++; // define variant in first pass
            else
                out << "Variant" << counter - 1; // reference in second pass
            break;
        }
        default:
            out << "byte[]";
            break;
        }
        return out.str();
    }

    // Oneof handling, phase-aware
    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::ostringstream out;
        if (ctx.pass == WalkContext::Pass::Flatten)
            out << "type " << capitalize(oneof.name) << " =\n"
                << "  | ...\n"; // placeholder
        else
            out << capitalize(oneof.name) << ": " << capitalize(oneof.name) << "\n"; // reference
        return out.str();
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
