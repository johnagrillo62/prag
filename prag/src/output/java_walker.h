// java_walker.h - Fixed Java code generator
#pragma once
#include "registry_ast_walker.h"
namespace bhw
{
class JavaAstWalker : public RegistryAstWalker
{
  private:
    struct VariantDef
    {
        std::string interfaceName;
        std::vector<std::pair<std::string, std::string>> types; // {javaType, recordName}
    };
    std::vector<VariantDef> variantDefs_;
    int variantCounter_ = 0;

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }

    std::string generateVariantDefinitions() const
    {
        if (variantDefs_.empty())
            return "";

        std::ostringstream out;
        out << "// Variant type definitions\n";

        for (const auto& def : variantDefs_)
        {
            // Build permits clause
            std::ostringstream permits;
            for (size_t i = 0; i < def.types.size(); ++i)
            {
                if (i > 0)
                    permits << ", ";
                permits << def.types[i].second;
            }

            // Generate sealed interface
            out << "public sealed interface " << def.interfaceName << " permits " << permits.str()
                << " {}\n\n";

            // Generate record types
            for (const auto& [javaType, recordName] : def.types)
            {
                out << "public record " << recordName << "(" << javaType << " value) implements "
                    << def.interfaceName << " {}\n";
            }
            out << "\n";
        }
        return out.str();
    }

  public:
    JavaAstWalker() : RegistryAstWalker(bhw::Language::Java)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Java;
    }

    std::string walk(bhw::Ast&& ast)
    {
        variantDefs_.clear();
        variantCounter_ = 0;
        std::string result = RegistryAstWalker::walk(std::move(ast));
        return result + generateVariantDefinitions();
    }

    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "import java.util.*;\n";
        out << "import java.time.*;\n";
        out << "import java.math.*;\n\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "public class " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {

        std::ostringstream out;

        std::string javaName = toCamelCase(field.name);

        out << indent(ind) << "public " << walkType(*field.type, ind) << " " << javaName << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "public enum " << e.name << " {\n";
        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool isLast, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << val.name << "(" << val.number << ")";
        out << (isLast ? ";\n\n" : ",\n");

        if (isLast)
        {
            out << indent(ind) << "private final int value;\n";
            out << indent(ind) << val.name.substr(0, val.name.find('_'))
                << "(int value) { this.value = value; }\n";
            out << indent(ind) << "public int getValue() { return value; }\n";
        }

        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace&, size_t) override
    {
        return ""; // Java uses package at file level
    }

    std::string generateNamespaceClose(const bhw::Namespace&, size_t) override
    {
        return "";
    }

    std::string generatePointerType(const PointerType& type, size_t ind = 0) override
    {
        std::string result = walkType(*type.pointee, ind);
        return result;
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return type.value->name;
    }

    std::string generateGenericType(const GenericType& type, size_t ind = 0) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
            out << "List<" << boxType(walkType(*type.args[0], ind)) << ">";
            break;

        case ReifiedTypeId::Set:
            out << "Set<" << boxType(walkType(*type.args[0], ind)) << ">";
            break;

        case ReifiedTypeId::Map:
            out << "Map<" << boxType(walkType(*type.args[0], ind)) << ", "
                << boxType(walkType(*type.args[1], ind)) << ">";
            break;

        case ReifiedTypeId::Optional:
            out << "Optional<" << boxType(walkType(*type.args[0], ind)) << ">";
            break;

        case ReifiedTypeId::Variant:
        {
            std::string interfaceName = "Variant" + std::to_string(variantCounter_++);

            VariantDef def;
            def.interfaceName = interfaceName;

            for (const auto& arg : type.args)
            {
                std::string javaType = walkType(*arg, ind);
                std::string recordName = interfaceName + boxType(capitalize(javaType));
                def.types.push_back({javaType, recordName});
            }

            variantDefs_.push_back(def);
            out << interfaceName;
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
        return canonicalToJava(type.reifiedType);
    }

    std::string generateOneof(const Oneof&, size_t) override
    {
        return "";
    }

  private:
    std::string canonicalToJava(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "boolean";
        case ReifiedTypeId::Int8:
            return "byte";
        case ReifiedTypeId::Int16:
            return "short";
        case ReifiedTypeId::Int32:
            return "int";
        case ReifiedTypeId::Int64:
            return "long";
        case ReifiedTypeId::UInt8:
            return "byte";
        case ReifiedTypeId::UInt16:
            return "short";
        case ReifiedTypeId::UInt32:
            return "int";
        case ReifiedTypeId::UInt64:
            return "long";
        case ReifiedTypeId::Float32:
            return "float";
        case ReifiedTypeId::Float64:
            return "double";
        case ReifiedTypeId::String:
            return "String";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::Bytes:
            return "byte[]";
        default:
            return "byte[]";
        }
    }

    std::string boxType(const std::string& type)
    {
        if (type == "int")
            return "Integer";
        if (type == "long")
            return "Long";
        if (type == "short")
            return "Short";
        if (type == "byte")
            return "Byte";
        if (type == "float")
            return "Float";
        if (type == "double")
            return "Double";
        if (type == "boolean")
            return "Boolean";
        if (type == "char")
            return "Character";
        return type;
    }

    std::string toCamelCase(const std::string& snake)
    {
        std::string result;
        bool capitalizeNext = false;

        for (char c : snake)
        {
            if (c == '_')
            {
                capitalizeNext = true;
            }
            else if (capitalizeNext)
            {
                result += static_cast<char>(std::toupper(c));
                capitalizeNext = false;
            }
            else
            {
                result += c;
            }
        }

        return result;
    }
};
} // namespace bhw