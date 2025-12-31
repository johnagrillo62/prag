#pragma once
#include "registry_ast_walker.h"

namespace bhw
{
class PythonAstWalker : public RegistryAstWalker
{
  public:
    PythonAstWalker() : RegistryAstWalker(Language::Python)
    {
    }

    Language getLang() override
    {
        return Language::Python;
    }

  protected:
    std::string generateHeader(const Ast&) override
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

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = static_cast<char>(std::toupper(r[0]));
        return r;
    }

    // ---------------- ONEOF ----------------
    void walkStruct(const Struct& s, std::ostringstream& out, size_t ind)
    {
        // Emit oneof variant classes first
        for (const auto& member : s.members)
        {
            if (auto oneof = std::get_if<Oneof>(&member))
            {
                for (const auto& field : oneof->fields)
                {
                    std::string clsName = capitalize(oneof->name) + capitalize(field.name);
                    out << indent(ind) << "@dataclass\n";
                    out << indent(ind) << "class " << clsName << ":\n";
                    out << indent(ind + 1) << "value: " << walkType(*field.type) << "\n\n";
                }
            }
        }

        // Emit the parent struct
        out << generateStructOpen(s, ind);

        for (const auto& member : s.members)
        {
            if (auto field = std::get_if<Field>(&member))
            {
                out << generateField(*field, ind + 1);
            }
            else if (auto oneof = std::get_if<Oneof>(&member))
            {
                out << indent(ind + 1) << "# Oneof: " << oneof->name << "\n";
                out << indent(ind + 1) << oneof->name << ": Union[";
                for (size_t i = 0; i < oneof->fields.size(); ++i)
                {
                    if (i > 0)
                        out << ", ";
                    out << capitalize(oneof->name) << capitalize(oneof->fields[i].name);
                }
                out << "]\n";
            }
        }

        out << generateStructClose(s, ind);
    }

    std::string generateOneof(const Oneof&, size_t) override
    {
        // Handled inline in walkStruct
        return "";
    }
};
} // namespace bhw
