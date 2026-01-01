// ocaml_walker.h - OCaml code generator using AST walker
#pragma once
#include <unordered_set>

#include "registry_ast_walker.h"

namespace bhw
{
class OCamlAstWalker : public RegistryAstWalker
{
  private:
    struct VariantDef
    {
        std::string name;
        std::vector<std::pair<std::string, std::string>> cases;
    };
    std::vector<VariantDef> variantDefs_;
    int variantCounter_ = 0;

    struct OneofDef
    {
        std::string name;
        std::vector<std::pair<std::string, std::string>> cases;
    };
    std::vector<OneofDef> oneofDefs_;

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
        // OCaml reserved keywords - add underscore suffix
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

    std::string generateVariantDefinitions() const
    {
        if (variantDefs_.empty() && oneofDefs_.empty())
            return "";
        std::ostringstream out;

        for (const auto& def : variantDefs_)
        {
            out << "type " << lowercase(def.name) << " =\n";
            for (const auto& [caseName, mlType] : def.cases)
                out << "  | " << caseName << " of " << mlType << "\n";
            out << "\n";
        }

        for (const auto& def : oneofDefs_)
        {
            out << "type " << lowercase(def.name) << " =\n";
            for (const auto& [fieldName, mlType] : def.cases)
                out << "  | " << capitalize(fieldName) << " of " << mlType << "\n";
            out << "\n";
        }
        return out.str();
    }

  public:
    OCamlAstWalker() : RegistryAstWalker(bhw::Language::OCaml)
    {
    }
    Language getLang() override
    {
        return bhw::Language::OCaml;
    }

    std::string walk(bhw::Ast&& ast)
    {
        // Flatten nested types first (like Go does)
        ast.flattenNestedTypes();

        variantDefs_.clear();
        oneofDefs_.clear();
        variantCounter_ = 0;

        // Walk the flattened AST
        std::string result = RegistryAstWalker::walk(std::move(ast));
        return generateVariantDefinitions() + result;
    }

    // Override to handle top-level Oneof nodes
    std::string walkRootNode(const bhw::AstRootNode& node, const WalkContext& ctx) override
    {
        // Handle top-level Oneof - add to collection for preamble generation
        if (std::holds_alternative<Oneof>(node))
        {
            const auto& oneof = std::get<Oneof>(node);
            std::string name = lowercase(oneof.name);
            OneofDef def;
            def.name = name;
            for (const auto& field : oneof.fields)
                def.cases.push_back({field.name, walkType(*field.type, ctx)});
            oneofDefs_.push_back(def);
            return ""; // Will be generated in the preamble
        }

        // Use default behavior for everything else
        return RegistryAstWalker::walkRootNode(node, ctx);
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "(* Generated OCaml types *)\n\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        return ctx.indent() + "type " + lowercase(s.name) + " = {\n";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << lowercase(field.name) << " : " << walkType(*field.type, ctx) << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        return ctx.indent() + "type " + lowercase(e.name) + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << "| " << capitalize(val.name) << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctz) override
    {
        return "\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, const WalkContext& ctx) override
    {
        return ctx.indent() + "module " + capitalize(ns.name) + " = struct\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, const WalkContext& ctx) override
    {
        return ctx.indent() + "end\n\n";
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
        // Always lowercase struct references in OCaml
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
        case ReifiedTypeId::Variant:
        {
            std::string name = "variant" + std::to_string(variantCounter_++);
            VariantDef def;
            def.name = name;
            for (size_t i = 0; i < type.args.size(); ++i)
            {
                std::string mlType = walkType(*type.args[i], ctx);
                def.cases.push_back({"V" + std::to_string(i), mlType});
            }
            variantDefs_.push_back(def);
            out << name;
            break;
        }
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
            return "int";
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

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::string name = lowercase(oneof.name);
        OneofDef def;
        def.name = name;
        for (const auto& field : oneof.fields)
            def.cases.push_back({field.name, walkType(*field.type, ctx)});
        oneofDefs_.push_back(def);
        std::ostringstream out;
        out << ctx.indent() << lowercase(oneof.name) << " : " << name << ";\n";
        return out.str();
    }
};
} // namespace bhw