#pragma once
#include <sstream>
#include <string>

#include "ast.h"
#include "language_info.h"
#include "languages.h"

namespace bhw
{

struct WalkContext
{
    enum class Pass
    {
        First,
        Normal
    };
    std::string indent() const
    {
        return std::string(level * 2, ' ');
    }
    std::string indent(size_t more) const
    {
        return std::string((level + more) * 2, ' ');
    }

    WalkContext nest() const
    {
        WalkContext ctx = *this;
        ctx.level++;
        return ctx;
    }
    WalkContext nest(size_t more) const
    {
        WalkContext ctx = *this;
        ctx.level += (more + 1);
        return ctx;
    }

    Pass pass = Pass::Normal;
    size_t level = 0;
    // Optional future fields:
    // std::string currentNamespace;
    // std::string parentStructName;
};

class AstWalker
{
  public:
    std::string srcLang;

    virtual ~AstWalker() = default;
    virtual auto getLang() -> Language = 0;

    virtual std::string walk(bhw::Ast&& ast)
    {
        NestedTypesPolicy policy = NestedTypesPolicy::NoNestedTypes;
        
        std::ostringstream out;

        out << generateHeader(ast);

        if (policy == NestedTypesPolicy::NoNestedTypes)
        {
            for (const auto& node : ast.nodes)
            {
                out << walkRootNode(node,
                                    WalkContext{.pass = WalkContext::Pass::First, .level = 0});
            };
        }

        for (const auto& node : ast.nodes)
        {
            out << walkRootNode(node, WalkContext{.pass = WalkContext::Pass::Normal, .level = 0});
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
    virtual auto walkRootNode(const bhw::AstRootNode& node, const WalkContext& ctx) -> std::string
    {
        return std::visit(
            [this, &node, ctx](auto&& n) -> std::string
            {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, Enum>)
                {
                    return walkEnum(n, ctx);
                }
                else if constexpr (std::is_same_v<T, Struct>)
                {
                    return walkStruct(n, ctx);
                }
                else if constexpr (std::is_same_v<T, bhw::Namespace>)
                {
                    return walkNamespace(n, ctx);
                }
                else if constexpr (std::is_same_v<T, bhw::Service>)
                {
                    return "";
                }

                else if constexpr (std::is_same_v<T, Oneof>)
                {
                    return walkOneof(n, ctx);
                }
                else
                {
                    static_assert(always_false_v<T>, "Unhandled type in walkRootNode!");
                }
            },
            node);
    }

    virtual std::string walkNamespace(const bhw::Namespace& ns, const WalkContext& ctx)
    {
        std::ostringstream out;

        out << generateNamespaceOpen(ns, ctx);

        // Walk all nodes in namespace
        for (const auto& node : ns.nodes)
        {
            out << walkRootNode(node, ctx.nest());
        }

        out << generateNamespaceClose(ns, ctx);

        return out.str();
    }
    virtual std::string walkStruct(const Struct& s, const WalkContext& ctx)
    {
        std::ostringstream out;
        out << generateStructOpen(s, ctx);

        for (auto& member : s.members)
        {
            out << walkStructMember(member, ctx.nest());
        }

        out << generateStructClose(s, ctx);
        return out.str();
    }

    virtual std::string walkStructMember(const StructMember& member, const WalkContext& ctx)
    {
        return std::visit(
            [this, ctx](auto&& m) -> std::string
            {
                using T = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<T, Field>)
                    return walkField(m, ctx);
                else if constexpr (std::is_same_v<T, Oneof>)
                    return walkOneof(m, ctx);
                else if constexpr (std::is_same_v<T, Enum>)
                    return walkEnum(m, ctx);
                else if constexpr (std::is_same_v<T, Struct>)
                    return walkStruct(m, ctx);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkStructMember!");
            },
            member);
    }

    virtual std::string walkField(const Field& field, const WalkContext& ctx)
    {
        return generateField(field, ctx);
    }

    virtual std::string walkEnum(const Enum& e, const WalkContext& ctx)
    {
        std::ostringstream out;

        out << generateEnumOpen(e, ctx);

        for (size_t i = 0; i < e.values.size(); ++i)
        {
            out << generateEnumValue(e.values[i], i == e.values.size() - 1, ctx.nest());
        }

        out << generateEnumClose(e, ctx);

        return out.str();
    }

    virtual std::string walkOneof(const Oneof& oneof, const WalkContext& ctx)
    {
        return generateOneof(oneof, ctx);
    }

    virtual std::string walkType(const Type& type, const WalkContext& ctx = WalkContext{})
    {
        return std::visit(
            [this, ctx](auto&& t) -> std::string
            {
                using T = std::decay_t<decltype(t)>;
                if constexpr (std::is_same_v<T, SimpleType>)
                    return generateSimpleType(t, ctx);
                else if constexpr (std::is_same_v<T, StructRefType>)
                    return generateStructRefType(t, ctx);
                else if constexpr (std::is_same_v<T, PointerType>)
                    return generatePointerType(t, ctx);
                else if constexpr (std::is_same_v<T, GenericType>)
                    return generateGenericType(t, ctx);
                else if constexpr (std::is_same_v<T, StructType>)
                    return generateStructType(t, ctx);
                else if constexpr (std::is_same_v<T, Oneof>)
                    return generateOneof(t);
                else
                    static_assert(always_false_v<T>, "Unhandled type in walkType!");
            },
            type.value);
    }

    // Override these methods for specific language code generation
    virtual std::string generateNamespaceOpen(const bhw::Namespace&, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateNamespaceClose(const bhw::Namespace&, const WalkContext& ctx)
    {
        return "";
    }

    virtual std::string generateStructOpen(const Struct&, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateStructClose(const Struct&, const WalkContext& ctx)
    {
        return "";
    }

    virtual std::string generateField(const Field&, const WalkContext& ctx)
    {
        return "";
    }

    virtual std::string generateEnumOpen(const Enum&, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateEnumValue(const EnumValue&, bool, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateEnumClose(const Enum&, const WalkContext& ctx)
    {
        return "";
    }

    virtual std::string generateOneof(const Oneof&, const WalkContext& ctx) = 0;
    virtual std::string generateSimpleType(const SimpleType&, const WalkContext& ctx)
    {
        return "";
    }

    virtual std::string generateStructRefType(const StructRefType& s, const WalkContext& ctx)
    {
        return s.srcTypeString;
    }

    virtual std::string generatePointerType(const PointerType&, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateGenericType(const GenericType&, const WalkContext& ctx)
    {
        return "";
    }
    virtual std::string generateStructType(const StructType&, const WalkContext& ctx)
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
