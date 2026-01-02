#pragma once
#include <cctype>
#include <sstream>

#include "ast.h"
#include "registry_ast_walker.h"

namespace bhw
{
class CSharpAstWalker : public RegistryAstWalker
{
  private:
    const Struct* currentStruct_ = nullptr;

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = std::toupper(r[0]);
        return r;
    }

    std::string indent(size_t n) const
    {
        return std::string(n * 2, ' ');
    }

  public:
    CSharpAstWalker() : RegistryAstWalker(bhw::Language::CSharp)
    {
    }

    Language getLang() override
    {
        return bhw::Language::CSharp;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "using System;\nusing System.Collections.Generic;\n\n";
    }

    std::string walkStruct(const Struct& s, const WalkContext& ctx) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << generateStructOpen(s, ctx);
        for (const auto& member : s.members)
            out << walkStructMember(member, ctx.nest());
        out << generateStructClose(s, ctx);
        return out.str();
    }

    std::string walkStructMember(const StructMember& member, const WalkContext& ctx) override
    {
        return std::visit(
            [this, ctx](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                    return generateField(m, ctx);
                else if constexpr (std::is_same_v<T, Oneof>)
                    return generateOneof(m, ctx);
                else if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(m, ctx);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(m, ctx);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkStructMember!");
            },
            member);
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::ostringstream out;

        // Base class name = capitalize(oneof.name)
        std::string baseName = capitalize(oneof.name);

        // 1. Generate property in the parent struct
        out << ctx.indent() << "public " << baseName << " " << baseName << " { get; set; }\n";

        // 2. Generate abstract base class
        out << ctx.indent() << "public abstract class " << baseName << " { }\n";

        // 3. Generate concrete variant classes
        for (const auto& field : oneof.fields)
        {
            std::string variantName = baseName + capitalize(field.name);
            out << ctx.indent() << "public class " << variantName << " : " << baseName << "\n";
            out << ctx.indent() << "{\n";
            out << ctx.indent(1) << "public " << walkType(*field.type, ctx)
                << " Value { get; set; }\n";
            out << ctx.indent() << "}\n";
        }

        out << "\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << ctx.indent() << "public class " << s.name << "\n" << ctx.indent() << "{\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << "public " << walkType(*field.type, ctx) << " "
            << capitalize(field.name) << " { get; set; }\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        return ctx.indent() + "public enum " + e.name + "\n" + ctx.indent() + "{\n";
    }

    std::string generateEnumValue(const EnumValue& val,
                                  bool isLast,
                                  const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent(1) << val.name;
        if (!isLast)
            out << ",";
        out << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        return ctx.indent() + "}\n\n";
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
            return "short";
        case ReifiedTypeId::UInt16:
            return "ushort";
        case ReifiedTypeId::Int32:
            return "int";
        case ReifiedTypeId::UInt32:
            return "uint";
        case ReifiedTypeId::Int64:
            return "long";
        case ReifiedTypeId::UInt64:
            return "ulong";
        case ReifiedTypeId::Float32:
            return "float";
        case ReifiedTypeId::Float64:
            return "double";
        case ReifiedTypeId::String:
            return "string";
        default:
            return "object";
        }
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            return "List<" + walkType(*type.args[0], ctx) + ">";
        case ReifiedTypeId::Set:
            return "HashSet<" + walkType(*type.args[0], ctx) + ">";
        case ReifiedTypeId::Map:
            return "Dictionary<" + walkType(*type.args[0], ctx) + ", " +
                   walkType(*type.args[1], ctx) + ">";
        case ReifiedTypeId::Optional:
            return walkType(*type.args[0], ctx) + "?";
        default:
            return "object";
        }
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext&) override
    {
        return type.srcTypeString;
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx);
    }
};
} // namespace bhw