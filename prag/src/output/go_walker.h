// go_code_generator.h - Go code generator using AST walker
#pragma once
#include <set>

#include "registry_ast_walker.h"

namespace bhw
{
// Go Walker using registry
class GoAstWalker : public RegistryAstWalker
{
  private:
    // Track variant types to generate
    struct VariantDef
    {
        std::string interfaceName;
        std::vector<std::pair<std::string, std::string>> types; // {goType, wrapperName}
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

    std::string makeWrapperName(const std::string& typeName) const
    {
        // Handle pointer types like *string -> PtrString
        if (!typeName.empty() && typeName[0] == '*')
            return "Ptr" + capitalize(typeName.substr(1));
        // Handle slice types like []byte -> SliceBytes
        if (typeName.size() > 2 && typeName[0] == '[' && typeName[1] == ']')
            return "Slice" + capitalize(typeName.substr(2));
        return capitalize(typeName);
    }

    std::string generateVariantDefinitions() const
    {
        if (variantDefs_.empty())
            return "";

        std::ostringstream out;
        out << "// Variant type definitions\n";

        for (const auto& def : variantDefs_)
        {
            // Generate sealed interface
            out << "type " << def.interfaceName << " interface {\n";
            out << "\tis" << def.interfaceName << "()\n";
            out << "}\n\n";

            // Generate wrapper types with marker methods
            for (const auto& [goType, wrapperName] : def.types)
            {
                out << "type " << wrapperName << " " << goType << "\n";
                out << "func (" << wrapperName << ") is" << def.interfaceName << "() {}\n\n";
            }
        }
        return out.str();
    }

  public:
    GoAstWalker() : RegistryAstWalker(bhw::Language::Go)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Go;
    }
    std::string walk(bhw::Ast&& ast)
    {
        ast.flattenNestedTypes();
        variantDefs_.clear();
        variantCounter_ = 0;
        std::string result = RegistryAstWalker::walk(std::move(ast));
        return result + generateVariantDefinitions();
    }

  protected:
    std::string generateHeader(const bhw::Ast& ast) override
    {
        return "package main\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        if (s.isAnonymous && !s.variableName.empty())
        {
            std::string name = s.variableName;
            if (std::islower(name[0]))
                name[0] = std::toupper(name[0]);
            return indent(ind) + name + " struct {\n";
        }
        return indent(ind) + "type " + s.name + " struct {\n";
    }

    std::string generateStructClose(const Struct& s, size_t ind) override
    {
        std::string result = indent(ind) + "}";

        if (s.isAnonymous && !s.variableName.empty())
        {
            std::string name = s.variableName;
            if (std::islower(name[0]))
                name[0] = std::toupper(name[0]);
            if (name != s.variableName)
            {
                result += " `json:\"" + s.variableName + "\"`";
            }
            return result + "\n";
        }

        return result + "\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        std::string go_name = field.name;
        if (!go_name.empty() && std::islower(go_name[0]))
        {
            go_name[0] = static_cast<char>(std::toupper(go_name[0]));
        }
        out << indent(ind) << go_name << " " << walkType(*field.type, ind);

        std::vector<std::string> tags;

        for (const auto& attr : field.attributes)
        {
            tags.push_back(attr.value);
        }

        if (go_name != field.name)
        {
            tags.push_back("json:\"" + field.name + "\"");
        }

        if (!tags.empty())
        {
            out << " `";
            for (size_t i = 0; i < tags.size(); ++i)
            {
                if (i > 0)
                    out << " ";
                out << tags[i];
            }
            out << "`";
        }

        out << "\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        return indent(ind) + "type " + e.name + " int\n\n" + indent(ind) + "const (\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << val.name;
        out << (val.number == 0 ? " = iota" : " = " + std::to_string(val.number));
        out << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t ind) override
    {
        return indent(ind) + ")\n\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace&, size_t) override
    {
        return "";
    }
    std::string generateNamespaceClose(const bhw::Namespace&, size_t) override
    {
        return "";
    }

    std::string generatePointerType(const PointerType& type, size_t) override
    {
        return "*" + walkType(*type.pointee);
    }

    std::string generateStructType(const StructType& type, size_t ind) override
    {
        const Struct& s = *type.value;
        if (s.isAnonymous || s.name == "<anonymous>")
        {
            std::ostringstream out;
            out << "struct {\n";
            for (const auto& member : s.members)
            {
                if (std::holds_alternative<Field>(member))
                {
                    out << generateField(std::get<Field>(member), ind + 1);
                }
            }
            out << indent(ind) << "}";
            return out.str();
        }
        return s.name;
    }

    std::string generateGenericType(const GenericType& type, size_t ind) override
    {
        // Handle Variant as sealed interface
        if (type.reifiedType == ReifiedTypeId::Variant)
        {
            std::string interfaceName = "Variant" + std::to_string(variantCounter_++);

            VariantDef def;
            def.interfaceName = interfaceName;

            for (const auto& arg : type.args)
            {
                std::string goType = walkType(*arg, ind);
                std::string wrapperName = interfaceName + makeWrapperName(goType);
                def.types.push_back({goType, wrapperName});
            }

            variantDefs_.push_back(def);
            return interfaceName;
        }

        return RegistryAstWalker::generateGenericType(type, ind);
    }

    std::string generateOneof(const Oneof&, size_t) override
    {
        return "";
    }
};
} // namespace bhw

