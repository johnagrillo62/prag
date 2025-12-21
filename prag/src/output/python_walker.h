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
        return "from dataclasses import dataclass\nfrom typing import List, Dict, Optional, "
               "Union\n\n";
    }

    std::string generateStructOpen(const Struct& s, size_t indent) override
    {
        return this->indent(indent) + "@dataclass\n" + this->indent(indent) + "class " + s.name +
               ":\n";
    }

    std::string generateStructClose(const Struct&, size_t) override
    {
        return "\n";
    }

    std::string generateField(const Field& field, size_t indent) override
    {
        return this->indent(indent) + field.name + ": " + walkType(*field.type) + "\n";
    }

    std::string generateEnumOpen(const Enum& e, size_t indent) override
    {
        return this->indent(indent) + "class " + e.name + "(Enum):\n";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t indent) override
    {
        return this->indent(indent) + val.name + " = " + std::to_string(val.number) + "\n";
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

    std::string generateOneof(const Oneof&, size_t) override
    {
        return "";
    }
};
} // namespace bhw