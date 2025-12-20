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
    struct OneofDef
    {
        std::string baseName;
        std::vector<std::pair<std::string, std::string>> fields; // field name + type
    };
    std::vector<OneofDef> oneofDefs_;

    struct AnonymousStructDef
    {
        std::string name;
        const Struct* structPtr;
    };
    std::vector<AnonymousStructDef> anonymousStructs_;

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

    // Emit registered oneof abstract + concrete types
    std::string generateVariantDefinitions()
    {
        std::ostringstream out;

        // Oneofs
        for (const auto& def : oneofDefs_)
        {
            out << "public abstract record " << def.baseName << ";\n";
            for (const auto& [fieldName, csType] : def.fields)
            {
                out << "public record " << def.baseName << capitalize(fieldName) << "(" << csType
                    << " Value) : " << def.baseName << ";\n";
            }
            out << "\n";
        }

        // Anonymous structs
        for (const auto& def : anonymousStructs_)
        {
            out << "public class " << def.name << "\n{\n";
            for (const auto& member : def.structPtr->members)
            {
                if (std::holds_alternative<Field>(member))
                {
                    const auto& field = std::get<Field>(member);
                    out << "  public " << walkType(*field.type, 1) << " " << capitalize(field.name)
                        << " { get; set; }\n";
                }
            }
            out << "}\n\n";
        }

        return out.str();
    }

  public:
    CSharpAstWalker() : RegistryAstWalker(bhw::Language::CSharp)
    {
    }

    Language getLang() override
    {
        return bhw::Language::CSharp;
    }

    std::string walk(bhw::Ast&& ast)
    {
        ast.flattenNestedTypes();
        oneofDefs_.clear();
        anonymousStructs_.clear();

        std::string result = RegistryAstWalker::walk(std::move(ast));
        result += generateVariantDefinitions(); // append oneof / anonymous structs
        return result;
    }

  protected:
    std::string generateHeader(const bhw::Ast&) override
    {
        return "using System;\nusing System.Collections.Generic;\n\n";
    }

    std::string walkStruct(const Struct& s, size_t indent) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << generateStructOpen(s, indent);

        for (const auto& member : s.members)
            out << walkStructMember(member, indent + 1);

        out << generateStructClose(s, indent);
        return out.str();
    }

    std::string walkStructMember(const StructMember& member, size_t indent) override
    {
        return std::visit(
            [this, indent](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                    return generateField(m, indent);
                else if constexpr (std::is_same_v<T, Oneof>)
                    return walkOneof(m, indent);
                else if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(m, indent);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(m, indent);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkStructMember!");
            },
            member);
    }

    std::string generateOneof(const Oneof& oneof, size_t ind)
    {
        return "";
    }
    std::string walkOneof(const Oneof& oneof, size_t ind)
    {
        std::string baseName;
        if (currentStruct_ && !currentStruct_->name.empty())
            baseName = currentStruct_->name + capitalize(oneof.name);
        else
            baseName = capitalize(oneof.name);

        // Register the oneof for top-level generation
        OneofDef def;
        def.baseName = baseName;
        for (const auto& field : oneof.fields)
        {
            std::string csType = walkType(*field.type, ind);
            def.fields.push_back({field.name, csType});
        }
        oneofDefs_.push_back(def);

        // Emit property in the parent class
        std::ostringstream out;
        out << indent(ind) << "public " << baseName << " " << capitalize(oneof.name)
            << " { get; set; }\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        currentStruct_ = &s;
        std::ostringstream out;
        out << indent(ind) << "public class " << s.name << "\n" << indent(ind) << "{\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "public " << walkType(*field.type, ind) << " "
            << capitalize(field.name) << " { get; set; }\n";
        return out.str();
    }
};
} // namespace bhw
