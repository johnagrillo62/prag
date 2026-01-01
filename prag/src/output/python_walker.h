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
    // ---------------- HEADER ----------------
    std::string generateHeader(const Ast&) override
    {
        return "from dataclasses import dataclass\nfrom typing import Union\n\n";
    }

    // ---------------- STRUCT ----------------
    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        return ctx.indent() + "@dataclass\n" + ctx.indent() + "class " + s.name + ":\n";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return "\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        return ctx.indent() + field.name + ": " + walkType(*field.type) + "\n";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::ostringstream out;

        // Emit each Oneof field as a nested @dataclass
        for (const auto& field : oneof.fields)
        {
            std::string clsName = capitalize(oneof.name) + capitalize(field.name);
            out << ctx.indent(1) << "@dataclass\n";
            out << ctx.indent(1) << "class " << clsName << ":\n";
            out << ctx.indent(3) << "value: " << walkType(*field.type) << "\n\n";
        }

        // Emit the parent Union line
        out << ctx.indent( 1) << "# Oneof: " << oneof.name << "\n";
        out << ctx.indent( 1) << oneof.name << ": Union[";
        for (size_t i = 0; i < oneof.fields.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            out << capitalize(oneof.name) + capitalize(oneof.fields[i].name);
        }
        out << "]\n";

        return out.str();
    }

    // ---------------- TYPE WALKING ----------------
    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = static_cast<char>(std::toupper(r[0]));
        return r;
    }

    
    // ---------------- UTILS ----------------
    std::string indent(size_t level) const
    {
        return std::string(level * 2, ' ');
    }
};
} // namespace bhw
