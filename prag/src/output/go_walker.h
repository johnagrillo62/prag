

// go_code_generator.h - Go code generator using AST walker
#pragma once
#include <set>
#include <sstream>

#include "registry_ast_walker.h"

namespace bhw
{
// Go AST walker using registry
class GoAstWalker : public RegistryAstWalker
{
  private:
    int variantCounter_ = 0;

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return "";
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

  public:
    GoAstWalker() : RegistryAstWalker(bhw::Language::Go)
    {
    }

    Language getLang() override
    {
        return bhw::Language::Go;
    }

    std::string walk(bhw::Ast&& ast) override
    {
        ast.flattenNestedTypes();
        std::string result = RegistryAstWalker::walk(std::move(ast));
        return generateVariantDefinitions() + result;
    }

  protected:
    std::string generateHeader(const bhw::Ast& ast) override
    {
        return "package main\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        return indent(ind) + "type " + s.name + " struct {\n";
    }

    std::string generateStructClose(const Struct& s, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        std::string go_name = field.name;
        if (!go_name.empty() && std::islower(go_name[0]))
        {
            go_name[0] = std::toupper(go_name[0]);
        }

        out << indent(ind) << go_name << " " << walkType(*field.type, ind);

        std::vector<std::string> tags;
        for (const auto& attr : field.attributes)
            tags.push_back(attr.value);

        if (go_name != field.name)
            tags.push_back("json:\"" + field.name + "\"");

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
                    out << generateField(std::get<Field>(member), ind + 1);
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

            std::ostringstream out;
            out << "// Variant interface for " << interfaceName << "\n";
            out << "type " << interfaceName << " interface {\n";
            out << "    is" << interfaceName << "()\n";
            out << "}\n\n";

            // Wrapper types implementing the interface
            for (const auto& arg : type.args)
            {
                std::string goType = walkType(*arg, ind);
                std::string wrapperName = interfaceName + makeWrapperName(goType);
                out << "type " << wrapperName << " " << goType << "\n";
                out << "func (" << wrapperName << ") is" << interfaceName << "() {}\n\n";
            }

            return interfaceName;
        }

        return RegistryAstWalker::generateGenericType(type, ind);
    }

    // Fixed generateOneof
    std::string generateOneof(const Oneof& oneof, size_t ind)
    {
        std::ostringstream out;
        std::string i = indent(ind);

        // 1️⃣ Wrapper struct
        out << i << "type " //<< capitalize(oneof.parentStructName) 
            << " struct {\n";
        out << i << "    " << capitalize(oneof.name) << " " << capitalize(oneof.name)
            << "Variant\n";
        out << i << "}\n\n";

        // 2️⃣ Variant structs
        for (const auto& field : oneof.fields)
        {
            out << i << "type " << capitalize(oneof.name) << capitalize(field.name)
                << " struct {\n";
            out << i << "    Value string\n";
            out << i << "}\n\n";
        }

        // 3️⃣ Variant interface
        out << i << "type " << capitalize(oneof.name) << "Variant interface {\n";
        out << i << "    is" << capitalize(oneof.name) << "Variant()\n";
        out << i << "}\n\n";

        // 4️⃣ Interface implementations
        for (const auto& field : oneof.fields)
        {
            std::string structName = capitalize(oneof.name) + capitalize(field.name);
            out << "func (" << structName << ") is" << capitalize(oneof.name) << "Variant() {}\n\n";
        }

        return out.str();
    }

    // Helper to generate all variant interfaces at the end if needed
    std::string generateVariantDefinitions() const
    {
        return ""; // Can be used for extra variant defs if required
    }
};
} // namespace bhw
