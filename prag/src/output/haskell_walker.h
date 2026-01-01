// haskell_walker.h - Haskell code generator using AST walker
#pragma once
#include <set>

#include "registry_ast_walker.h"

namespace bhw
{
class HaskellAstWalker : public RegistryAstWalker
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

    struct AnonymousStructDef
    {
        std::string name;
        const Struct* structPtr;
    };
    std::vector<AnonymousStructDef> anonymousStructs_;

    // Track if we're on the last field for comma handling
    size_t currentFieldIndex_ = 0;
    size_t totalFields_ = 0;

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
        // Haskell reserved keywords
        static const std::set<std::string> reserved = {
            "case",     "class", "data",      "default", "deriving", "do",     "else",
            "foreign",  "if",    "import",    "in",      "infix",    "infixl", "infixr",
            "instance", "let",   "module",    "newtype", "of",       "then",   "type",
            "where",    "as",    "qualified", "hiding"};

        if (reserved.find(name) != reserved.end())
        {
            return name + "_"; // Add underscore suffix for reserved words
        }
        return name;
    }

    std::string generateVariantDefinitions()
    {
        if (variantDefs_.empty() && oneofDefs_.empty() && anonymousStructs_.empty())
            return "";
        std::ostringstream out;

        // Generate anonymous structs first
        for (const auto& def : anonymousStructs_)
        {
            out << "data " << def.name << " = " << def.name << "\n    { ";

            size_t fieldIdx = 0;
            for (const auto& member : def.structPtr->members)
            {
                if (std::holds_alternative<Field>(member))
                {
                    const auto& field = std::get<Field>(member);
                    if (fieldIdx > 0)
                        out << "\n        , ";
                    std::string fieldName = escapeReservedKeyword(lowercase(field.name));
                    out << fieldName << " :: " << walkType(*field.type, WalkContext{});
                    fieldIdx++;
                }
            }
            out << "\n        } deriving (Show, Eq)\n\n";
        }

        for (const auto& def : variantDefs_)
        {
            out << "data " << def.name << " =\n";
            for (size_t i = 0; i < def.cases.size(); ++i)
            {
                const auto& [caseName, hsType] = def.cases[i];
                out << "    " << (i == 0 ? "  " : "| ") << caseName << " " << hsType << "\n";
            }
            out << "    deriving (Show, Eq)\n\n";
        }

        for (const auto& def : oneofDefs_)
        {
            out << "data " << def.name << " =\n";
            for (size_t i = 0; i < def.cases.size(); ++i)
            {
                const auto& [fieldName, hsType] = def.cases[i];
                out << "    " << (i == 0 ? "  " : "| ") << capitalize(fieldName) << " " << hsType
                    << "\n";
            }
            out << "    deriving (Show, Eq)\n\n";
        }
        return out.str();
    }

  public:
    HaskellAstWalker() : RegistryAstWalker(bhw::Language::Haskell)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Haskell;
    }

    // Override walkRootNode to handle Oneof
    std::string walkRootNode(const bhw::AstRootNode& node, const WalkContext& ctx) override
    {
        return std::visit(
            [this, ctx](auto&& n) -> std::string
            {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(n, ctx);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(n, ctx);
                else if constexpr (std::is_same_v<T, bhw::Namespace>)
                    return walkNamespace(n, ctx);
                else if constexpr (std::is_same_v<T, Service>)
                    return "";
                else if constexpr (std::is_same_v<T, Oneof>)
                    return generateOneofAsType(n, ctx);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkRootNode!");
            },
            node);
    }

    std::string walk(bhw::Ast&& ast)
    {
        ast.flattenNestedTypes();
        variantDefs_.clear();
        oneofDefs_.clear();
        anonymousStructs_.clear();
        variantCounter_ = 0;
        std::string result = RegistryAstWalker::walk(std::move(ast));

        // Insert variant definitions after header, not before
        std::string variantDefs = generateVariantDefinitions();
        if (!variantDefs.empty())
        {
            // Find the end of the header (after "import qualified Data.Set as Set")
            size_t insertPos = result.find("import qualified Data.Set as Set\n");
            if (insertPos != std::string::npos)
            {
                // Move past this line and the following newlines
                insertPos = result.find('\n', insertPos);
                if (insertPos != std::string::npos)
                {
                    insertPos = result.find('\n', insertPos + 1);
                    if (insertPos != std::string::npos)
                    {
                        insertPos++; // Move past the newline
                        result.insert(insertPos, variantDefs);
                    }
                }
            }
        }

        return result;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "{-# LANGUAGE DeriveGeneric #-}\nmodule Generated where\n\n"
               "import Data.Int\n"
               "import Data.Word\n"
               "import Data.Text (Text)\n"
               "import Data.Time (UTCTime, Day, TimeOfDay, DiffTime)\n"
               "import Data.UUID (UUID)\n"
               "import Data.Scientific (Scientific)\n"
               "import qualified Data.Map as Map\n"
               "import qualified Data.Set as Set\n\n";
    }

    std::string walkStruct(const Struct& s, const WalkContext& ctx) override
    {
        std::ostringstream out;

        out << generateStructOpen(s, ctx);

        // Count actual fields (including oneofs)
        totalFields_ = 0;
        for (const auto& member : s.members)
        {
            if (std::holds_alternative<Field>(member) || std::holds_alternative<Oneof>(member))
                totalFields_++;
        }

        // Walk all members
        currentFieldIndex_ = 0;
        for (const auto& member : s.members)
        {
            out << walkStructMember(member, ctx.nest());
        }

        out << generateStructClose(s, ctx);

        return out.str();
    }

    std::string walkEnum(const Enum& e, const WalkContext& ctx) override
    {
        std::ostringstream out;

        out << generateEnumOpen(e, ctx);

        for (size_t i = 0; i < e.values.size(); ++i)
        {
            bool isFirst = (i == 0);
            const auto& val = e.values[i];

            if (isFirst)
            {
                out << ctx.indent() << "      " << capitalize(val.name) << "\n";
            }
            else
            {
                out << ctx.indent() << "    | " << capitalize(val.name) << "\n";
            }
        }

        out << generateEnumClose(e, ctx);

        return out.str();
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        return ctx.indent() + "data " + s.name + " = " + s.name + "\n" + ctx.indent() + "    { ";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return "} deriving (Show, Eq)\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;

        // Add comma before field if not the first
        if (currentFieldIndex_ > 0)
        {
            out << ", ";
        }

        std::string fieldName = escapeReservedKeyword(lowercase(field.name));
        out << fieldName << " :: " << walkType(*field.type, ctx) << "\n" << ctx.indent() << "      ";

        currentFieldIndex_++;

        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        return ctx.indent() + "data " + e.name + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool isFirst, const WalkContext& ctx) override
    {
        // Not used anymore - handled in walkEnum
        return "";
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        return ctx.indent() + "    deriving (Show, Eq, Enum)\n\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace&, const WalkContext& ctx) override
    {
        return "";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, const WalkContext& ctx) override
    {
        return "";
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return "IORef (" + walkType(*type.pointee, ctx) + ")";
    }

    std::string generateStructType(const StructType& type, const WalkContext& ctx) override
    {
        // Check if this is an anonymous struct
        if (type.value->isAnonymous || type.value->name.empty() ||
            type.value->name == "<anonymous>")
        {
            // Generate a unique name based on the variable name or counter
            std::string typeName;
            if (!type.value->variableName.empty())
            {
                typeName = capitalize(type.value->variableName) + "Type";
            }
            else
            {
                static int anonCounter = 0;
                typeName = "AnonymousType" + std::to_string(anonCounter++);
            }

            // Register this anonymous struct for generation
            AnonymousStructDef def;
            def.name = typeName;
            def.structPtr = type.value.get();
            anonymousStructs_.push_back(def);

            return typeName;
        }
        return type.value->name;
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << "[" << walkType(*type.args[0], ctx) << "]";
            break;
        case ReifiedTypeId::Set:
            out << "Set.Set " << walkType(*type.args[0], ctx);
            break;
        case ReifiedTypeId::Map:
            out << "Map.Map " << walkType(*type.args[0], ctx) << " "
                << walkType(*type.args[1], ctx);
            break;
        case ReifiedTypeId::Optional:
            out << "Maybe " << walkType(*type.args[0], ctx);
            break;
        case ReifiedTypeId::Variant:
        {
            std::string name = "Variant" + std::to_string(variantCounter_++);
            VariantDef def;
            def.name = name;
            for (size_t i = 0; i < type.args.size(); ++i)
            {
                std::string hsType = walkType(*type.args[i], ctx);
                def.cases.push_back({name + "V" + std::to_string(i), hsType});
            }
            variantDefs_.push_back(def);
            out << name;
            break;
        }
        default:
            out << "ByteString";
            break;
        }
        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext& ctx) override
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
        case ReifiedTypeId::Char:
            return "Char";
        case ReifiedTypeId::Bytes:
            return "ByteString";
        case ReifiedTypeId::DateTime:
            return "UTCTime";
        case ReifiedTypeId::Date:
            return "Day";
        case ReifiedTypeId::Time:
            return "TimeOfDay";
        case ReifiedTypeId::Duration:
            return "DiffTime";
        case ReifiedTypeId::UUID:
            return "UUID";
        case ReifiedTypeId::Decimal:
            return "Scientific";
        default:
            return "()";
        }
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext& ctx) override
    {
        return type.srcTypeString;
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::string name = capitalize(oneof.name);
        OneofDef def;
        def.name = name;
        for (const auto& field : oneof.fields)
            def.cases.push_back({field.name, walkType(*field.type, ctx)});
        oneofDefs_.push_back(def);

        std::ostringstream out;

        // Add comma before oneof if not the first field
        if (currentFieldIndex_ > 0)
        {
            out << ", ";
        }

        std::string fieldName = escapeReservedKeyword(lowercase(oneof.name));
        out << fieldName << " :: " << name << "\n" << ctx.indent() << "      ";

        currentFieldIndex_++;

        return out.str();
    }

    // Generate Oneof as a top-level data type
    std::string generateOneofAsType(const Oneof& oneof, const WalkContext& ctx)
    {
        std::ostringstream out;
        std::string name = capitalize(oneof.name);

        out << ctx.indent() << "data " << name << " =\n";
        for (size_t i = 0; i < oneof.fields.size(); ++i)
        {
            const auto& field = oneof.fields[i];
            out << ctx.indent() << "    " << (i == 0 ? "  " : "| ") << capitalize(field.name) << " "
                << walkType(*field.type, ctx) << "\n";
        }
        out << ctx.indent() << "    deriving (Show, Eq)\n\n";
        return out.str();
    }
};
} // namespace bhw