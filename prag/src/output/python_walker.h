#pragma once
#include "registry_ast_walker.h"

namespace bhw
{
class PythonAstWalker : public RegistryAstWalker
{
  public:
    PythonAstWalker() : RegistryAstWalker(Language::Python)
    {
    }

    Language getLang() override
    {
        return Language::Python;
    }

  protected:
    // ---------------- HEADER ----------------
    std::string generateHeader(const Ast&) override
    {
        return "from dataclasses import dataclass\nfrom typing import Union\n\n";
    }

    // ---------------- STRUCT ----------------
    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "@dataclass\n" + ctx.indent() + "class " + s.name + ":\n";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return "\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + field.name + ": " + walkType(*field.type, ctx) + "\n";
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "class " + e.name + ":\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent(1) + val.name + " = " + std::to_string(val.number) + "\n";
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return "\n";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            // Generate oneof variant classes during flatten pass
            std::ostringstream out;
            for (const auto& field : oneof.fields)
            {
                std::string clsName = capitalize(oneof.name) + capitalize(field.name);
                out << "@dataclass\n";
                out << "class " << clsName << ":\n";
                out << "  value: " << walkType(*field.type, ctx) << "\n\n";
            }
            return out.str();
        }
        else
        {
            // Generate Union field during normal pass
            std::ostringstream out;
            out << ctx.indent() << oneof.name << ": Union[";
            for (size_t i = 0; i < oneof.fields.size(); ++i)
            {
                if (i > 0)
                    out << ", ";
                out << capitalize(oneof.name) + capitalize(oneof.fields[i].name);
            }
            out << "]\n";
            return out.str();
        }
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext& ctx) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
        case ReifiedTypeId::Int16:
        case ReifiedTypeId::Int32:
        case ReifiedTypeId::Int64:
        case ReifiedTypeId::UInt8:
        case ReifiedTypeId::UInt16:
        case ReifiedTypeId::UInt32:
        case ReifiedTypeId::UInt64:
            return "int";
        case ReifiedTypeId::Float32:
        case ReifiedTypeId::Float64:
            return "float";
        case ReifiedTypeId::String:
            return "str";
        case ReifiedTypeId::Bytes:
            return "bytes";
        default:
            return "object";
        }
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << "list[" << walkType(*type.args[0], ctx) << "]";
            break;
        case ReifiedTypeId::Set:
            out << "set[" << walkType(*type.args[0], ctx) << "]";
            break;
        case ReifiedTypeId::Map:
            out << "dict[" << walkType(*type.args[0], ctx) << ", " << walkType(*type.args[1], ctx)
                << "]";
            break;
        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ctx) << " | None";
            break;
        default:
            out << "object";
            break;
        }
        return out.str();
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext& ctx) override
    {
        return type.srcTypeString;
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx);
    }

    // ---------------- UTILS ----------------
    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = static_cast<char>(std::toupper(r[0]));
        return r;
    }
};
} // namespace bhw