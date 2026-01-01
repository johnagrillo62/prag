#pragma once
#include <cctype>
#include <sstream>
#include <vector>

#include "registry_ast_walker.h"

namespace bhw
{
class JavaAstWalker : public RegistryAstWalker
{
  private:
    int oneofCounter_ = 0;

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return "";
        std::string r = s;
        r[0] = std::toupper(r[0]);
        return r;
    }

    std::string indent(size_t n) const
    {
        return std::string(n * 2, ' ');
    }

  public:
    JavaAstWalker() : RegistryAstWalker(Language::Java)
    {
    }

    Language getLang() override
    {
        return Language::Java;
    }

    std::string walk(bhw::Ast&& ast) override
    {
        return RegistryAstWalker::walk(std::move(ast));
    }

  protected:
    // Header imports
    std::string generateHeader(const bhw::Ast&) override
    {
        return "import java.util.*;\nimport java.time.*;\nimport java.math.*;\n\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << "public class " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << walkType(*field.type, ctx) << " " << capitalize(field.name) << ";\n";
        return out.str();
    }

    // Main override for oneof
    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        // Interface name = ParentStructName + OneofName
        
        std::string interfaceName = capitalize(oneof.parentStructName) + capitalize(oneof.name);

        // Generate property in the parent struct
        std::ostringstream out;
        out << ctx.indent() << interfaceName << " " << capitalize(oneof.name) << ";\n";



        // Sealed interface
        out << ctx.indent() << "public sealed interface " << interfaceName << " permits ";
        auto sep = "";
        for (const auto& field : oneof.fields)
        {
            out << sep << interfaceName << capitalize(field.name);
            sep = ", ";
        }
        out << " {}\n";

        // Record variants
        for (const auto& field : oneof.fields)
        {
            out << ctx.indent() << "public record " << interfaceName << capitalize(field.name) << "("
                << walkType(*field.type, ctx) << " value) implements " << interfaceName << " {}\n";
        }

        out << "\n";
        return out.str();
    }
};
} // namespace bhw
