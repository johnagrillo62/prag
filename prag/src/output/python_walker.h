#pragma once

#include "registry_ast_walker.h"

namespace bhw
{
// Python Walker using registry
class PythonAstWalker : public RegistryAstWalker
{
  public:
    PythonAstWalker() : RegistryAstWalker(bhw::Language::Python)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Python;
    }

  protected:
    std::string generateHeader(const bhw::Ast& ast) override
    {
        return "from dataclasses import dataclass\nfrom typing import Union\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        return indent(ind) + "@dataclass\n" + indent(ind) + "class " + s.name + ":\n";
    }

    std::string generateStructClose(const Struct&, size_t) override
    {
        return "\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        return indent(ind) + field.name + ": " + walkType(*field.type) + "\n";
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        return indent(ind) + "class " + e.name + "(Enum):\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        return indent(ind) + val.name + " = " + std::to_string(val.number) + "\n";
    }

    std::string generateEnumClose(const Enum&, size_t) override
    {
        return "\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace&, size_t) override
    {
        return "";
    }
    std::string generateNamespaceClose(const bhw::Namespace&, size_t) override
    {
        return "";
    }

    std::string generatePointerType(const PointerType& type, size_t indent) override
    {
        return "Optional[" + walkType(*type.pointee, indent) + "]";
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return type.value->name;
    }

    // ---------- FIXED ONEOF ----------
    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::ostringstream topLevelClasses;

        // Generate top-level dataclasses for each variant
        for (const auto& field : oneof.fields)
        {
            std::string clsName = capitalize(oneof.name) + capitalize(field.name);
            topLevelClasses << "@dataclass\n";
            topLevelClasses << "class " << clsName << ":\n";
            topLevelClasses << "    value: " << walkType(*field.type) << "\n\n";
        }

        // Generate the oneof field in the parent struct
        std::ostringstream parentField;
        parentField << indent(ind) << "# Oneof: " << oneof.name << "\n";
        parentField << indent(ind) << oneof.name << ": Union<";
        for (size_t i = 0; i < oneof.fields.size(); ++i)
        {
            if (i > 0)
                parentField << ", ";
            parentField << capitalize(oneof.name) << capitalize(oneof.fields[i].name);
        }
        parentField << ">\n";

        return topLevelClasses.str() + parentField.str();
    }

    // Helper to capitalize names
    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = static_cast<char>(std::toupper(r[0]));
        return r;
    }
};
} // namespace bhw
