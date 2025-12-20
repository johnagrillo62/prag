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
    std::string walkRootNode(const bhw::AstRootNode& node, size_t indent) override
    {
        // Handle top-level Oneof - add to collection for preamble generation
        if (std::holds_alternative<Oneof>(node))
        {
            const auto& oneof = std::get<Oneof>(node);
            std::string name = lowercase(oneof.name);
            OneofDef def;
            def.name = name;
            for (const auto& field : oneof.fields)
                def.cases.push_back({field.name, walkType(*field.type, 0)});
            oneofDefs_.push_back(def);
            return ""; // Will be generated in the preamble
        }

        // Use default behavior for everything else
        return RegistryAstWalker::walkRootNode(node, indent);
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "(* Generated OCaml types *)\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        return indent(ind) + "type " + lowercase(s.name) + " = {\n";
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << lowercase(field.name) << " : " << walkType(*field.type, ind) << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        return indent(ind) + "type " + lowercase(e.name) + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "| " << capitalize(val.name) << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t) override
    {
        return "\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, size_t ind) override
    {
        return indent(ind) + "module " + capitalize(ns.name) + " = struct\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, size_t ind) override
    {
        return indent(ind) + "end\n\n";
    }

    std::string generatePointerType(const PointerType& type, size_t ind = 0) override
    {
        return walkType(*type.pointee, ind) + " ref";
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return lowercase(type.value->name);
    }

    std::string generateStructRefType(const StructRefType& type, size_t) override
    {
        // Always lowercase struct references in OCaml
        return lowercase(type.srcTypeString);
    }

    std::string generateGenericType(const GenericType& type, size_t ind = 0) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << walkType(*type.args[0], ind) << " list";
            break;
        case ReifiedTypeId::Set:
            out << walkType(*type.args[0], ind) << " Set.t";
            break;
        case ReifiedTypeId::Map:
            out << "(" << walkType(*type.args[0], ind) << ", " << walkType(*type.args[1], ind)
                << ") Map.t";
            break;
        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ind) << " option";
            break;
        case ReifiedTypeId::Variant:
        {
            std::string name = "variant" + std::to_string(variantCounter_++);
            VariantDef def;
            def.name = name;
            for (size_t i = 0; i < type.args.size(); ++i)
            {
                std::string mlType = walkType(*type.args[i], ind);
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

    std::string generateSimpleType(const SimpleType& type, size_t) override
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

    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::string name = lowercase(oneof.name);
        OneofDef def;
        def.name = name;
        for (const auto& field : oneof.fields)
            def.cases.push_back({field.name, walkType(*field.type, ind)});
        oneofDefs_.push_back(def);
        std::ostringstream out;
        out << indent(ind) << lowercase(oneof.name) << " : " << name << ";\n";
        return out.str();
    }
};
} // namespace bhw