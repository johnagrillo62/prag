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

    std::string walk(bhw::Ast&& ast) override
    {
        // Flatten nested types FIRST - this hoists anonymous structs to top level
        ast.flattenNestedTypes();
        // Reset counter
        fieldCounter_ = 0;

        // Now walk normally
        return RegistryAstWalker::walk(std::move(ast));
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "@0xdbb9ad1f14bf0b36;\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        fieldCounter_ = 0; // Reset for each struct
        return indent(ind) + "struct " + s.name + " {\n";
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
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

        out << indent(ind) << field.name << " @" << fieldNum << " :" << walkType(*field.type, ind)
            << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        return indent(ind) + "enum " + e.name + " {\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << val.name << " @" << val.number << ";\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    // CRITICAL: Override walkStructMember to skip nested structs
    // Cap'n Proto doesn't support nested struct definitions
    std::string walkStructMember(const StructMember& member, size_t indent) override
    {
        return std::visit(
            [this, indent](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                {
                    return walkField(m, indent);
                }
                else if constexpr (std::is_same_v<T, Oneof>)
                {
                    return generateOneof(m, indent);
                }
                else if constexpr (std::is_same_v<T, Enum>)
                {
                    return walkEnum(m, indent);
                }
                else if constexpr (std::is_same_v<T, Struct>)
                {
                    // Skip nested struct definitions!
                    // They should have been flattened to top level already
                    return "";
                }
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkStructMember!");
            },
            member);
    }

    std::string generateSimpleType(const SimpleType& type, size_t) override
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
            return "Data"; // Unknown types map to Data
        }
    }

    std::string generateGenericType(const GenericType& type, size_t ind) override
    {
        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            return "List(" + walkType(*type.args[0], ind) + ")";
        case ReifiedTypeId::Optional:
            // Cap'n Proto doesn't have explicit Optional, just use the type
            return walkType(*type.args[0], ind);
        case ReifiedTypeId::Map:
            // Cap'n Proto doesn't have native maps, use List of pairs
            return "List(Data)"; // Simplified
        default:
            return "Data";
        }
    }

    std::string generateStructRefType(const StructRefType& type, size_t) override
    {
        return type.srcTypeString;
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        // Return the struct name
        if (type.value && !type.value->name.empty())
        {
            return type.value->name;
        }
        return "Data"; // Fallback
    }

    std::string generatePointerType(const PointerType& type, size_t ind) override
    {
        // Cap'n Proto doesn't have explicit pointers, just use the type
        return walkType(*type.pointee, ind);
    }

    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::ostringstream out;

        out << indent(ind) << "union " << oneof.name << "{\n";

        for (const auto& field : oneof.fields)
        {
            out << indent(ind + 1) << field.name;

            for (const auto& attr : field.attributes)
            {
                if (attr.name == "field_number")
                {
                    out << " @" << attr.value << ": " << walkType(*field.type);
                   
                }
                out << ";\n";
            }
        }
        out << indent(ind) << "}\n";
        return out.str();
    }
};
} // namespace bhw
