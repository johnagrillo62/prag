// fsharp_walker.h - F# code generator using AST walker
#pragma once
#include "registry_ast_walker.h"

namespace bhw
{
class FSharpAstWalker : public RegistryAstWalker
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

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    std::string generateVariantDefinitions()
    {
        if (variantDefs_.empty() && oneofDefs_.empty() && anonymousStructs_.empty())
            return "";
        std::ostringstream out;

        // Generate anonymous struct types first
        for (const auto& def : anonymousStructs_)
        {
            out << "type " << def.name << " = {\n";
            for (const auto& member : def.structPtr->members)
            {
                if (std::holds_alternative<Field>(member))
                {
                    const auto& field = std::get<Field>(member);
                    out << "  " << capitalize(field.name) << ": " << walkType(*field.type, 1)
                        << "\n";
                }
            }
            out << "}\n\n";
        }

        for (const auto& def : variantDefs_)
        {
            out << "type " << def.name << " =\n";
            for (const auto& [caseName, fsType] : def.cases)
                out << "    | " << caseName << " of " << fsType << "\n";
            out << "\n";
        }

        for (const auto& def : oneofDefs_)
        {
            out << "type " << def.name << " =\n";
            for (const auto& [fieldName, fsType] : def.cases)
                out << "    | " << capitalize(fieldName) << " of " << fsType << "\n";
            out << "\n";
        }
        return out.str();
    }

  public:
    FSharpAstWalker() : RegistryAstWalker(bhw::Language::FSharp)
    {
    }
    Language getLang() override
    {
        return bhw::Language::FSharp;
    }

    // Override walkRootNode to handle Oneof
    std::string walkRootNode(const bhw::AstRootNode& node, size_t indent) override
    {
        return std::visit(
            [this, indent](auto&& n) -> std::string
            {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(n, indent);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(n, indent);
                else if constexpr (std::is_same_v<T, bhw::Namespace>)
                    return walkNamespace(n, indent);
                else if constexpr (std::is_same_v<T, Service>)
                    return "";
                else if constexpr (std::is_same_v<T, Oneof>)
                    return generateOneofAsType(n, indent); // Generate discriminated union
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

        // Insert variant definitions after header (inside namespace), not before it
        std::string variantDefs = generateVariantDefinitions();
        if (!variantDefs.empty())
        {
            // Find the end of the header (after "open" statements)
            // Header format: "namespace Generated\n\nopen System\nopen
            // System.Collections.Generic\n\n"
            size_t insertPos = result.find("open System.Collections.Generic\n");
            if (insertPos != std::string::npos)
            {
                // Move past this line and the following newline
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
        return "namespace Generated\n\nopen System\nopen System.Collections.Generic\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        return indent(ind) + "type " + s.name + " = {\n";
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << capitalize(field.name) << ": " << walkType(*field.type, ind) << "\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        return indent(ind) + "type " + e.name + " =\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "| " << val.name << " = " << val.number << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t) override
    {
        return "\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, size_t ind) override
    {
        // Top-level namespaces (ind==0) are already declared in header, so don't generate "module"
        // Only nested namespaces should generate "module" declarations
        if (ind == 0)
            return "";
        return indent(ind) + "module " + ns.name + " =\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, size_t) override
    {
        return "\n";
    }

    std::string generatePointerType(const PointerType& type, size_t ind = 0) override
    {
        return walkType(*type.pointee, ind);
    }

    std::string generateStructType(const StructType& type, size_t ind) override
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

    std::string generateGenericType(const GenericType& type, size_t ind = 0) override
    {
        std::ostringstream out;
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << walkType(*type.args[0], ind) << " list";
            break;
        case ReifiedTypeId::Array:
            out << walkType(*type.args[0], ind) << "[]";
            break;
        case ReifiedTypeId::Set:
            out << "Set<" << walkType(*type.args[0], ind) << ">";
            break;
        case ReifiedTypeId::Map:
            out << "Map<" << walkType(*type.args[0], ind) << ", " << walkType(*type.args[1], ind)
                << ">";
            break;
        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ind) << " option";
            break;
        case ReifiedTypeId::Variant:
        {
            std::string name = "Variant" + std::to_string(variantCounter_++);
            VariantDef def;
            def.name = name;
            for (size_t i = 0; i < type.args.size(); ++i)
            {
                std::string fsType = walkType(*type.args[i], ind);
                def.cases.push_back({"Case" + std::to_string(i), fsType});
            }
            variantDefs_.push_back(def);
            out << name;
            break;
        }
        default:
            out << "byte[]";
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

    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::string name = capitalize(oneof.name);
        OneofDef def;
        def.name = name;
        for (const auto& field : oneof.fields)
            def.cases.push_back({field.name, walkType(*field.type, ind)});
        oneofDefs_.push_back(def);
        std::ostringstream out;
        out << indent(ind) << capitalize(oneof.name) << ": " << name << "\n";
        return out.str();
    }

    // Generate Oneof as a top-level discriminated union type
    std::string generateOneofAsType(const Oneof& oneof, size_t ind)
    {
        std::ostringstream out;
        out << indent(ind) << "type " << oneof.name << " =\n";
        for (const auto& field : oneof.fields)
        {
            out << indent(ind + 1) << "| " << capitalize(field.name) << " of "
                << walkType(*field.type, ind) << "\n";
        }
        out << "\n";
        return out.str();
    }
};
} // namespace bhw 
  