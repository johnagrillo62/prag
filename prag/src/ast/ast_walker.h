#pragma once
#include <sstream>
#include <string>

#include "ast.h"
#include "languages.h"

namespace bhw
{

class AstWalker
{
  public:

    std::string srcLang;
  
    virtual ~AstWalker() = default;
    virtual auto getLang() -> Language = 0;

    // Main entry point - walk the entire AST
    virtual std::string walk(bhw::Ast&& ast)
    {
        std::ostringstream out;

        // Generate file header
        out << generateHeader(ast);

        // Walk all root nodes
        for (const auto& node : ast.nodes)
        {
            out << walkRootNode(node, 0);
        }

        // Generate file footer
        out << generateFooter(ast);

        return out.str();
    }

    // Override these to customize code generation
    virtual std::string generateHeader(const bhw::Ast&)
    {
        return "";
    }
    virtual std::string generateFooter(const bhw::Ast&)
    {
        return "";
    }

    // Walk different node types
    virtual auto walkRootNode(const bhw::AstRootNode& node, size_t indent) -> std::string
    {
        return std::visit(
            [this, &node, indent](auto&& n) -> std::string
            {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(n, indent);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(n, indent);
                else if constexpr (std::is_same_v<T, bhw::Namespace>)
                    return walkNamespace(n, indent);
                else if constexpr (std::is_same_v<T, bhw::Service>)
                    return "";

                else if constexpr (std::is_same_v<T, Oneof>)
                {
                    return "";
                }
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkRootNode!");
            },
            node);
    }

    virtual std::string walkNamespace(const bhw::Namespace& ns, size_t indent)
    {
        std::ostringstream out;

        out << generateNamespaceOpen(ns, indent);

        // Walk all nodes in namespace
        for (const auto& node : ns.nodes)
        {
            out << walkRootNode(node, indent + 1);
        }

        out << generateNamespaceClose(ns, indent);

        return out.str();
    }

    virtual std::string walkStruct(const Struct& s, size_t indent)
    {
        std::ostringstream out;

        out << generateStructOpen(s, indent);

        // Walk all members
        for (const auto& member : s.members)
        {
            out << walkStructMember(member, indent + 1);
        }

        out << generateStructClose(s, indent);

        return out.str();
    }

    virtual std::string walkStructMember(const StructMember& member, size_t indent)
    {
        return std::visit(
            [this, indent](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                    return walkField(m, indent);
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

    virtual std::string walkField(const Field& field, size_t indent)
    {
        return generateField(field, indent);
    }

    virtual std::string walkEnum(const Enum& e, size_t indent)
    {
        std::ostringstream out;

        out << generateEnumOpen(e, indent);

        for (size_t i = 0; i < e.values.size(); ++i)
        {
            out << generateEnumValue(e.values[i], i == e.values.size() - 1, indent + 1);
        }

        out << generateEnumClose(e, indent);

        return out.str();
    }

    virtual std::string walkOneof(const Oneof& oneof, size_t indent)
    {
        return generateOneof(oneof, indent);
    }

    virtual std::string walkType(const Type& type, size_t ind = 0)
    {
        return std::visit(
            [this, ind](auto&& t) -> std::string
            {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, SimpleType>)
                    return generateSimpleType(t, ind);
                else if constexpr (std::is_same_v<T, StructRefType>)
                    return generateStructRefType(t, ind);
                else if constexpr (std::is_same_v<T, PointerType>)
                    return generatePointerType(t, ind);
                else if constexpr (std::is_same_v<T, GenericType>)
                    return generateGenericType(t, ind);
                else if constexpr (std::is_same_v<T, StructType>)
                    return generateStructType(t, ind);
                else if constexpr (std::is_same_v<T, Oneof>)
                    return generateOneof(t);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkType!");
            },
            type.value);
    }

    // Override these methods for specific language code generation
    virtual std::string generateNamespaceOpen(const bhw::Namespace&, size_t)
    {
        return "";
    }
    virtual std::string generateNamespaceClose(const bhw::Namespace&, size_t)
    {
        return "";
    }

    virtual std::string generateStructOpen(const Struct&, size_t)
    {
        return "";
    }
    virtual std::string generateStructClose(const Struct&, size_t)
    {
        return "";
    }

    virtual std::string generateField(const Field&, size_t)
    {
        return "";
    }

    virtual std::string generateEnumOpen(const Enum&, size_t)
    {
        return "";
    }
    virtual std::string generateEnumValue(const EnumValue&, bool, size_t)
    {
        return "";
    }
    virtual std::string generateEnumClose(const Enum&, size_t)
    {
        return "";
    }

    virtual std::string generateOneof(const Oneof&, size_t) = 0;
    virtual std::string generateSimpleType(const SimpleType&, size_t)
    {
        return "";
    }

    virtual std::string generateStructRefType(const StructRefType& s, size_t)
    {
        return s.srcTypeString;
    }

    virtual std::string generatePointerType(const PointerType&, size_t)
    {
        return "";
    }
    virtual std::string generateGenericType(const GenericType&, size_t)
    {
        return "";
    }
    virtual std::string generateStructType(const StructType&, size_t)
    {
        return "";
    }

    // Utility methods
    std::string indent(size_t level) const
    {
        return std::string(level * 2, ' ');
    }

    std::string getAttributeValue(const std::vector<Attribute>& attrs,
                                  const std::string& name,
                                  const std::string& defaultValue = "") const
    {
        for (const auto& attr : attrs)
        {
            if (attr.name == name)
                return attr.value;
        }
        return defaultValue;
    }

    bool hasAttribute(const std::vector<Attribute>& attrs, const std::string& name) const
    {
        for (const auto& attr : attrs)
        {
            if (attr.name == name)
                return true;
        }
        return false;
    }


};
} // namespace bhw
