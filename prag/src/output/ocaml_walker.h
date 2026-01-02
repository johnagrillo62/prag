// ocaml_walker.h - OCaml code generator using AST walker
#pragma once
#include <unordered_set>

#include "registry_ast_walker.h"

namespace bhw
{
class OCamlAstWalker : public RegistryAstWalker
{
  private:
    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    std::string lowercase(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::tolower(result[0]);
        return escapeKeyword(result);
    }

    std::string escapeKeyword(const std::string& s) const
    {
        static const std::unordered_set<std::string> keywords = {
            "and",     "as",      "assert",      "asr",      "begin",   "class",     "constraint",
            "do",      "done",    "downto",      "else",     "end",     "exception", "external",
            "false",   "for",     "fun",         "function", "functor", "if",        "in",
            "include", "inherit", "initializer", "land",     "lazy",    "let",       "lor",
            "lsl",     "lsr",     "lxor",        "match",    "method",  "mod",       "module",
            "mutable", "new",     "nonrec",      "object",   "of",      "open",      "or",
            "private", "rec",     "sig",         "struct",   "then",    "to",        "true",
            "try",     "type",    "val",         "virtual",  "when",    "while",     "with"};

        if (keywords.find(s) != keywords.end())
            return s + "_";
        return s;
    }

  public:
    OCamlAstWalker() : RegistryAstWalker(bhw::Language::OCaml)
    {
    }

    Language getLang() override
    {
        return bhw::Language::OCaml;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "(* Generated OCaml types *)\n\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return ctx.indent() + "type " + lowercase(s.name) + " = {\n";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";

        std::ostringstream out;
        out << ctx.indent() << lowercase(field.name) << " : " << walkType(*field.type, ctx)
            << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return ctx.indent() + "type " + lowercase(e.name) + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";

        std::ostringstream out;
        out << ctx.indent() << "| " << capitalize(val.name) << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return "\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return ctx.indent() + "module " + capitalize(ns.name) + " = struct\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return ctx.indent() + "end\n\n";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            // ✅ Generate oneof variant type during flatten pass
            std::ostringstream out;
            std::string name = lowercase(oneof.name);
            out << "type " << name << " =\n";
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
            // ✅ Generate field during normal pass
            std::ostringstream out;
            out << ctx.indent() << lowercase(oneof.name) << " : " << lowercase(oneof.name) << ";\n";
            return out.str();
        }
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx) + " ref";
    }

    std::string generateStructType(const StructType& type, const WalkContext& ctx) override
    {
        return lowercase(type.value->name);
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext& ctx) override
    {
        return lowercase(type.srcTypeString);
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << walkType(*type.args[0], ctx) << " list";
            break;
        case ReifiedTypeId::Set:
            out << walkType(*type.args[0], ctx) << " Set.t";
            break;
        case ReifiedTypeId::Map:
            out << "(" << walkType(*type.args[0], ctx) << ", " << walkType(*type.args[1], ctx)
                << ") Map.t";
            break;
        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ctx) << " option";
            break;
        default:
            out << "bytes";
            break;
        }
        return out.str();
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
        case ReifiedTypeId::UInt8:
        case ReifiedTypeId::UInt16:
        case ReifiedTypeId::UInt32:
            return "int";
        case ReifiedTypeId::Int64:
        case ReifiedTypeId::UInt64:
            return "int64";
        case ReifiedTypeId::Float32:
        case ReifiedTypeId::Float64:
            return "float";
        case ReifiedTypeId::String:
            return "string";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::Bytes:
            return "bytes";
        default:
            return "unit";
        }
    }
};
} // namespace bhw