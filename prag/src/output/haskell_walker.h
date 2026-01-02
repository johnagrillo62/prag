// haskell_walker.h - Haskell code generator using AST walker
#pragma once
#include <set>

#include "registry_ast_walker.h"

namespace bhw
{
class HaskellAstWalker : public RegistryAstWalker
{
  private:
    size_t currentFieldIndex_ = 0;

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
        return result;
    }

    std::string escapeReservedKeyword(const std::string& name) const
    {
        static const std::set<std::string> reserved = {
            "case",     "class", "data",      "default", "deriving", "do",     "else",
            "foreign",  "if",    "import",    "in",      "infix",    "infixl", "infixr",
            "instance", "let",   "module",    "newtype", "of",       "then",   "type",
            "where",    "as",    "qualified", "hiding"};

        if (reserved.find(name) != reserved.end())
            return name + "_";
        return name;
    }

  public:
    Language getLang() override
    {
        return bhw::Language::Haskell;
    }
    HaskellAstWalker() : RegistryAstWalker(bhw::Language::Haskell)
    {
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        currentFieldIndex_ = 0;

        return "{-# LANGUAGE DeriveGeneric #-}\nmodule Generated where\n\n"
               "import Data.Int\n"
               "import Data.Word\n"
               "import Data.Text (Text)\n"
               "import qualified Data.Map as Map\n"
               "import qualified Data.Set as Set\n\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";

        currentFieldIndex_ = 0;
        return "data " + s.name + " = " + s.name + "\n    { ";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return "} deriving (Show, Eq)\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";

        std::ostringstream out;
        if (currentFieldIndex_ > 0)
            out << ", ";

        std::string fieldName = escapeReservedKeyword(lowercase(field.name));
        out << fieldName << " :: " << walkType(*field.type, ctx) << "\n      ";
        currentFieldIndex_++;
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return "data " + e.name + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val,
                                  bool isFirst,
                                  const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";

        return (isFirst ? "      " : "    | ") + capitalize(val.name) + "\n";
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
            return "";
        return "    deriving (Show, Eq, Enum)\n\n";
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext&) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::Bool:
            return "Bool";
        case ReifiedTypeId::Int8:
            return "Int8";
        case ReifiedTypeId::UInt8:
            return "Word8";
        case ReifiedTypeId::Int16:
            return "Int16";
        case ReifiedTypeId::UInt16:
            return "Word16";
        case ReifiedTypeId::Int32:
            return "Int32";
        case ReifiedTypeId::UInt32:
            return "Word32";
        case ReifiedTypeId::Int64:
            return "Int64";
        case ReifiedTypeId::UInt64:
            return "Word64";
        case ReifiedTypeId::Float32:
            return "Float";
        case ReifiedTypeId::Float64:
            return "Double";
        case ReifiedTypeId::String:
            return "Text";
        default:
            return "()";
        }
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            return "[" + walkType(*type.args[0], ctx) + "]";
        case ReifiedTypeId::Set:
            return "Set.Set " + walkType(*type.args[0], ctx);
        case ReifiedTypeId::Map:
            return "Map.Map " + walkType(*type.args[0], ctx) + " " + walkType(*type.args[1], ctx);
        case ReifiedTypeId::Optional:
            return "Maybe " + walkType(*type.args[0], ctx);
        default:
            return "()";
        }
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext&) override
    {
        return type.srcTypeString;
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            // Generate oneof type directly during flatten pass
            std::ostringstream out;
            std::string name = capitalize(oneof.name);
            out << "data " << name << " =\n";
            for (size_t i = 0; i < oneof.fields.size(); ++i)
            {
                const auto& field = oneof.fields[i];
                out << "    " << (i == 0 ? "  " : "| ") << capitalize(field.name) << " "
                    << walkType(*field.type, ctx) << "\n";
            }
            out << "    deriving (Show, Eq)\n\n";
            return out.str();
        }
        else
        {
            // Generate field during normal pass
            std::ostringstream out;
            if (currentFieldIndex_ > 0)
                out << ", ";

            std::string name = capitalize(oneof.name);
            std::string fieldName = escapeReservedKeyword(lowercase(oneof.name));
            out << fieldName << " :: " << name << "\n      ";
            currentFieldIndex_++;
            return out.str();
        }
    }
};
} // namespace bhw