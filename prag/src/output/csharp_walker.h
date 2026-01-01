#pragma once
#include <cctype>
#include <sstream>

#include "ast.h"
#include "registry_ast_walker.h"

namespace bhw
{
class CSharpAstWalker : public RegistryAstWalker
{
  private:
    const Struct* currentStruct_ = nullptr;

    std::string capitalize(const std::string& s) const
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = std::toupper(r[0]);
        return r;
    }

    std::string indent(size_t n) const
    {
        return std::string(n * 2, ' ');
    }

  public:
    CSharpAstWalker() : RegistryAstWalker(bhw::Language::CSharp)
    {
    }

    Language getLang() override
    {
        return bhw::Language::CSharp;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "using System;\nusing System.Collections.Generic;\n\n";
    }

    std::string walkStruct(const Struct& s, const WalkContext& ctx) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << generateStructOpen(s, ctx);

        for (const auto& member : s.members)
            out << walkStructMember(member, ctx.nest());

        out << generateStructClose(s, ctx);
        return out.str();
    }

    std::string walkStructMember(const StructMember& member, const WalkContext& ctx) override
    {
        return std::visit(
            [this, ctx](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                    return generateField(m, ctx); // properties with getters/setters
                else if constexpr (std::is_same_v<T, Oneof>)
                    return walkOneof(m, ctx);
                else if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(m, ctx);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(m, ctx); // nested structs are fully named now
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkStructMember!");
            },
            member);
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::ostringstream out;
        std::string baseName =
            currentStruct_ ? currentStruct_->name + capitalize(oneof.name) : capitalize(oneof.name);

        // property in the parent class, with getter/setter
        out << ctx.indent() << "public " << baseName << " " << capitalize(oneof.name)
            << " { get; set; }\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << ctx.indent() << "public class " << s.name << "\n" << ctx.indent() << "{\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        std::ostringstream out;
        // preserve getter/setter style
        out << ctx.indent() << "public " << walkType(*field.type, ctx) << " "
            << capitalize(field.name) << " { get; set; }\n";
        return out.str();
    }
};
} // namespace bhw
