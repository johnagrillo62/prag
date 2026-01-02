#pragma once
#include "registry_ast_walker.h"

namespace bhw
{
class RustAstWalker : public RegistryAstWalker
{
  public:
    RustAstWalker() : RegistryAstWalker(bhw::Language::Rust)
    {
    }

    Language getLang() override
    {
        return bhw::Language::Rust;
    }

    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "#![allow(dead_code)]\n\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip during flatten pass
        }

        std::ostringstream out;
        out << ctx.indent() << "#[derive(Debug, Clone)]\n";
        out << ctx.indent() << "pub struct " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip during flatten pass
        }

        std::ostringstream out;
        out << ctx.indent() << "pub " << field.name << ": " << walkType(*field.type, ctx) << ",\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip during flatten pass
        }

        std::ostringstream out;

        // Check if this is a variant-style enum (has types associated with values)
        bool hasAssociatedTypes = false;
        for (const auto& val : e.values)
        {
            if (val.type)
            {
                hasAssociatedTypes = true;
                break;
            }
        }

        out << ctx.indent() << "#[derive(Debug, Clone";
        if (!hasAssociatedTypes)
        {
            out << ", Copy, PartialEq, Eq";
        }
        out << ")]\n";
        out << ctx.indent() << "pub enum " << e.name << " {\n";
        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;

        // If the enum value has an associated type, generate Rust-style variant with data
        if (val.type)
        {
            out << ctx.indent() << val.name << "(" << walkType(*val.type, ctx) << "),\n";
        }
        else
        {
            // Otherwise, generate C-style enum value
            out << ctx.indent() << val.name << " = " << val.number << ",\n";
        }

        return out.str();
    }

    std::string generateEnumClose(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << ctx.indent() << "}\n\n";
        return out.str();
    }

    std::string generateNamespaceOpen(const Namespace& ns, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "pub mod " + ns.name + " {\n";
    }

    std::string generateNamespaceClose(const Namespace& ns, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "} // mod " + ns.name + "\n\n";
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return "Box<" + walkType(*type.pointee, ctx) + ">";
    }

    std::string generateStructType(const StructType& type, const WalkContext& ctx) override
    {
        return type.value->name;
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
        case ReifiedTypeId::Set:
            out << "Vec<" << walkType(*type.args[0], ctx) << ">";
            break;

        case ReifiedTypeId::Map:
            out << "std::collections::HashMap<" << walkType(*type.args[0], ctx) << ", "
                << walkType(*type.args[1], ctx) << ">";
            break;

        case ReifiedTypeId::Optional:
            out << "Option<" << walkType(*type.args[0], ctx) << ">";
            break;

        default:
            out << "Vec<u8>";
            break;
        }

        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext& ctx) override
    {
        return canonicalToRust(type.reifiedType);
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            // Generate Rust enum during flatten pass
            std::ostringstream out;
            out << "#[derive(Debug, Clone)]\n";
            out << "pub enum " << oneof.name << " {\n";

            for (const auto& field : oneof.fields)
            {
                out << "  " << field.name << "(" << walkType(*field.type, ctx) << "),\n";
            }

            out << "}\n\n";
            return out.str();
        }
        else
        {
            // Generate field in struct during normal pass
            std::ostringstream out;
            out << ctx.indent() << "pub " << oneof.name << ": " << oneof.name << ",\n";
            return out.str();
        }
    }

  private:
    std::string canonicalToRust(ReifiedTypeId type)
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
            return "String";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::Bytes:
            return "Vec<u8>";
        default:
            return "Vec<u8>";
        }
    }
};

} // namespace bhw