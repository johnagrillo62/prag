#pragma once
#include <variant>

#include "registry_ast_walker.h"

namespace bhw
{
class CppWalker : public RegistryAstWalker
{
  public:
    CppWalker() : RegistryAstWalker(bhw::Language::Cpp26)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Cpp26;
    }

    std::string walk(bhw::Ast&& ast) override
    {
        this->srcLang = ast.srcName;
        // C++ needs enums before structs that use them
        std::vector<AstRootNode> enums;
        std::vector<AstRootNode> structs;

        for (auto& node : ast.nodes)
        {
            if (std::holds_alternative<Enum>(node))
            {
                enums.push_back(std::move(node));
            }
            else if (std::holds_alternative<Struct>(node))
            {
                // Extract nested enums from struct members
                auto& s = std::get<Struct>(node);
                extractNestedEnums(s.members, enums);
                structs.push_back(std::move(node));
            }
            else
            {
                structs.push_back(std::move(node));
            }
        }

        // Rebuild: enums first, then everything else
        ast.nodes.clear();
        for (auto& e : enums)
            ast.nodes.push_back(std::move(e));
        for (auto& s : structs)
            ast.nodes.push_back(std::move(s));

        return RegistryAstWalker::walk(std::move(ast));
    }

  private:
    void extractNestedEnums(std::vector<StructMember>& members, std::vector<AstRootNode>& enums)
    {
        auto it = members.begin();
        while (it != members.end())
        {
            if (std::holds_alternative<Enum>(*it))
            {
                enums.push_back(std::move(std::get<Enum>(*it)));
                it = members.erase(it);
            }
            else if (std::holds_alternative<Struct>(*it))
            {
                extractNestedEnums(std::get<Struct>(*it).members, enums);
                ++it;
            }
            else
            {
                ++it;
            }
        }
    }

  protected:
    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "#pragma once\n"
            << "#include <cstdint>\n"
            << "#include <string>\n"
            << "#include <vector>\n"
            << "#include <map>\n"
            << "#include <variant>\n"
            << "#include <optional>\n"
            << "\n";
        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext& ctx) override
    {
        // If source language == target language, preserve exact spelling
        if (srcLang == "h")
        {
            if (!type.srcTypeString.empty())
            {
                return type.srcTypeString;
            }
        }

        // Map to C type if srcTypeString is blank
        switch (type.reifiedType)
        {
        case ReifiedTypeId::Int8:
            return "int8_t";
        case ReifiedTypeId::Int16:
            return "int16_t";
        case ReifiedTypeId::Int32:
            return "int32_t";
        case ReifiedTypeId::Int64:
            return "int64_t";
        case ReifiedTypeId::UInt8:
            return "uint8_t";
        case ReifiedTypeId::UInt16:
            return "uint16_t";
        case ReifiedTypeId::UInt32:
            return "uint32_t";
        case ReifiedTypeId::UInt64:
            return "uint64_t";
        case ReifiedTypeId::Float32:
            return "float";
        case ReifiedTypeId::Float64:
            return "double";
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::String:
            return "std::string";
        case ReifiedTypeId::Bytes:
            return "std::vector<uint8_t>";
        case ReifiedTypeId::DateTime:
            return "std::chrono::system_clock::time_point"; // Add this

        default:
            return "void"; // fallback
        }
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx)  override
    {
        std::string name = s.isAnonymous ? "" : s.name;

        return ctx.indent() + "struct " + name + "\n" + ctx.indent() + "{\n";
    }

    std::string generateStructClose(const Struct& s, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << "}";
        if (!s.variableName.empty())
            out << " " << s.variableName;
        out << ";\n\n";
        return out.str();
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        return ctx.indent(namespaces.size()) + walkType(*field.type, ctx) + " " + field.name +
               ";\n";
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent() << "enum ";
        if (e.scoped)
            out << "class ";

        out << e.name;
        if (!e.underlying_type.empty())
            out << " : " << e.underlying_type << " ";

        out << "\n";
        out << ctx.indent() << "{\n";
        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val,
                                  bool isLast,
                                  const WalkContext& ctx) override
    {
        std::ostringstream out;
        out << ctx.indent(namespaces.size()) << val.name;
        if (!isLast)
            out << ",";
        out << "\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        return ctx.indent(namespaces.size()) + "};\n\n";
    }

    std::string generateNamespaceOpen(const bhw::Namespace& ns, const WalkContext& ctx) override
    {
        namespaces.emplace_back(ns.name);
        return ctx.indent() + "namespace " + ns.name + "\n" + ctx.indent() + "{\n";
    }

    std::string generateNamespaceClose(const bhw::Namespace& ns, const WalkContext& ctx) override
    {
        namespaces.pop_back();

        return ctx.indent() + "} // namespace " + ns.name + "\n\n";
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx) + "*";
    }

    std::string generateStructType(const StructType& type, const WalkContext& ctx) override
    {
        const Struct& s = *type.value;
        // If it's an anonymous inline struct, generate it inline
        if (s.isAnonymous || s.name == "<anonymous>")
        {
            std::ostringstream out;
            out << "struct\n" << ctx.indent() << "{\n";

            // Generate fields at next indent level
            for (const auto& member : s.members)
            {
                if (std::holds_alternative<Field>(member))
                {
                    const auto& field = std::get<Field>(member);
                    out << generateField(field, ctx.nest(namespaces.size()));
                }
            }

            out << ctx.indent() << "}";
            return out.str();
        }

        // Named struct, just return the name
        return s.name;
    }

  protected:
    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        std::ostringstream out;

        out << ctx.indent(namespaces.size()) << "// Oneof: " << oneof.name << "\n";

        // Generate wrapper structs
        for (const auto& field : oneof.fields)
        {
            std::string wrapperName = capitalize(oneof.name) + "_" + capitalize(field.name);
            std::string fieldType = walkType(*field.type, ctx);

            out << ctx.indent(namespaces.size()) << "struct " << wrapperName << " {\n"
                << ctx.indent(namespaces.size() + 1) << fieldType << " value;\n"
                << ctx.indent(namespaces.size()) << "};\n\n";
        }

        // Generate the variant field directly (no type alias)
        out << indent(namespaces.size()) << "std::variant<std::monostate";

        for (const auto& field : oneof.fields)
        {
            std::string wrapperName = capitalize(oneof.name) + "_" + capitalize(field.name);
            out << ", " << wrapperName;
        }

        out << "> " << oneof.name << ";\n\n";

        return out.str();
    }

  private:
    std::string capitalize(const std::string& s)
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(static_cast<unsigned char>(result[0]));
        return result;
    }

  protected:
    void modifyAst(bhw::Ast& ast)
    {
        // Extract nested enums from structs and hoist to top level
        std::vector<AstRootNode> hoisted;

        for (auto& node : ast.nodes)
        {
            if (std::holds_alternative<Struct>(node))
            {
                auto& s = std::get<Struct>(node);
                hoistEnums(s, hoisted);
            }
        }

        // Insert hoisted enums at front
        ast.nodes.insert(ast.nodes.begin(),
                         std::make_move_iterator(hoisted.begin()),
                         std::make_move_iterator(hoisted.end()));
    }

  private:
    void hoistEnums(Struct& s, std::vector<AstRootNode>& hoisted)
    {
        auto it = s.members.begin();
        while (it != s.members.end())
        {
            if (std::holds_alternative<Enum>(*it))
            {
                hoisted.push_back(std::move(std::get<Enum>(*it)));
                it = s.members.erase(it);
            }
            else if (std::holds_alternative<Struct>(*it))
            {
                hoistEnums(std::get<Struct>(*it), hoisted);
                ++it;
            }
            else
            {
                ++it;
            }
        }
    }

  private:
    std::vector<std::string> namespaces;
};
} // namespace bhw
