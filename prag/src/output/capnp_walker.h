// capnp_walker.h - Cap'n Proto code generator
#pragma once
#include <sstream>

#include "ast.h"
#include "registry_ast_walker.h"

namespace bhw
{
class CapnProtoAstWalker : public RegistryAstWalker
{
  private:
    int fieldCounter_ = 0;

  public:
    CapnProtoAstWalker() : RegistryAstWalker(bhw::Language::Capnp)
    {
    }

    Language getLang() override
    {
        return bhw::Language::Capnp;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "@0xdbb9ad1f14bf0b36;\n\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Skip during flatten pass
        }

        fieldCounter_ = 0; // Reset for each struct
        return ctx.indent() + "struct " + s.name + " {\n";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
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
            return ""; // Skip fields during flatten pass
        }

        std::ostringstream out;

        // Find field number from attributes
        int fieldNum = fieldCounter_++;
        for (const auto& attr : field.attributes)
        {
            if (attr.name == "field_number")
            {
                try
                {
                    fieldNum = std::stoi(attr.value);
                }
                catch (...)
                {
                }
                break;
            }
        }

        out << ctx.indent() << field.name << " @" << fieldNum << " :" << walkType(*field.type, ctx)
            << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "enum " + e.name + " {\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << ctx.indent() << val.name << " @" << val.number << ";\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "}\n\n";
    }

    std::string generateSimpleType(const SimpleType& type, const bhw::WalkContext& ctx) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::Bool:
            return "Bool";
        case ReifiedTypeId::Int8:
            return "Int8";
        case ReifiedTypeId::UInt8:
            return "UInt8";
        case ReifiedTypeId::Int16:
            return "Int16";
        case ReifiedTypeId::UInt16:
            return "UInt16";
        case ReifiedTypeId::Int32:
            return "Int32";
        case ReifiedTypeId::UInt32:
            return "UInt32";
        case ReifiedTypeId::Int64:
            return "Int64";
        case ReifiedTypeId::UInt64:
            return "UInt64";
        case ReifiedTypeId::Float32:
            return "Float32";
        case ReifiedTypeId::Float64:
            return "Float64";
        case ReifiedTypeId::String:
            return "Text";
        case ReifiedTypeId::Bytes:
            return "Data";
        default:
            return "Data";
        }
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            return "List(" + walkType(*type.args[0], ctx) + ")";
        case ReifiedTypeId::Optional:
            return walkType(*type.args[0], ctx);
        case ReifiedTypeId::Map:
            return "List(Data)";
        default:
            return "Data";
        }
    }

    std::string generateStructRefType(const StructRefType& type,
                                      const bhw::WalkContext& ctx) override
    {
        return type.srcTypeString;
    }

    std::string generateStructType(const StructType& type, const bhw::WalkContext& ctx) override
    {
        if (type.value && !type.value->name.empty())
        {
            return type.value->name;
        }
        return "Data";
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx);
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return ""; // Cap'n Proto doesn't need flatten pass for oneofs
        }

        std::ostringstream out;
        out << ctx.indent() << "union " << oneof.name << " {\n";

        int unionFieldNum = 0;
        for (const auto& field : oneof.fields)
        {
            out << ctx.indent(1) << field.name << " @" << unionFieldNum++ << " :"
                << walkType(*field.type, ctx) << ";\n";
        }

        out << ctx.indent() << "}\n";
        return out.str();
    }
};
} // namespace bhw