// protobuf_generator.h - Protocol Buffers code generator using AST walker
#pragma once
#include <stack>

#include "registry_ast_walker.h"

namespace bhw
{
class ProtoBufAstWalker : public RegistryAstWalker
{
  public:
    ProtoBufAstWalker() : RegistryAstWalker(bhw::Language::ProtoBuf)
    {
    }

    Language getLang() override
    {
        return bhw::Language::ProtoBuf;
    }

  private:
    int field_number_ = 1;
    std::stack<int> field_number_stack_;

    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "syntax = \"proto3\";\n\n";

        if (!ast.namespaces.empty())
        {
            out << "package ";
            for (size_t i = 0; i < ast.namespaces.size(); ++i)
            {
                if (i > 0)
                    out << ".";
                out << ast.namespaces[i];
            }
            out << ";\n\n";
        }

        return out.str();
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        field_number_stack_.push(field_number_);
        field_number_ = 1;

        out << ctx.indent() << "message " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        if (!field_number_stack_.empty())
        {
            field_number_ = field_number_stack_.top();
            field_number_stack_.pop();
        }
        return ctx.indent() + "}\n\n";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;

        // Check if this field is a Variant type -> generate oneof
        if (std::holds_alternative<GenericType>(field.type->value))
        {
            const auto& gt = std::get<GenericType>(field.type->value);
            if (gt.reifiedType == ReifiedTypeId::Variant)
            {
                out << ctx.indent() << "oneof " << field.name << " {\n";

                int oneof_field_num = field_number_;
                for (const auto& arg : gt.args)
                {
                    std::string typeName = walkType(*arg, ctx);
                    std::string fieldName = field.name + "_" + typeName;
                    out << ctx.indent(1) << typeName << " " << fieldName << " = "
                        << oneof_field_num++ << ";\n";
                }

                out << ctx.indent() << "}\n";
                field_number_ = oneof_field_num;
                return out.str();
            }
        }

        // Normal field
        out << ctx.indent() << walkType(*field.type, ctx) << " " << field.name << " = "
            << field_number_++ << ";\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << ctx.indent() << "enum " << e.name << " {\n";

        if (e.values.empty() || e.values[0].number != 0)
        {
            out << ctx.indent(1) << e.name << "_UNSPECIFIED = 0;\n";
        }

        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << ctx.indent() << val.name << " = " << val.number << ";\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }
        return ctx.indent() + "}\n\n";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (ctx.pass == WalkContext::Pass::Flatten)
        {
            return "";
        }

        std::ostringstream out;
        out << ctx.indent() << "oneof " << oneof.name << " {\n";

        int oneof_field_num = 1;
        for (const auto& field : oneof.fields)
        {
            out << ctx.indent(1) << walkType(*field.type, ctx) << " " << field.name << " = "
                << oneof_field_num++ << ";\n";
        }

        out << ctx.indent() << "}\n";
        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, const WalkContext& ctx) override
    {
        return canonicalToProtobuf(type.reifiedType);
    }

    std::string generatePointerType(const PointerType& type, const WalkContext& ctx) override
    {
        return walkType(*type.pointee, ctx);
    }

    std::string generateGenericType(const GenericType& type, const WalkContext& ctx) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
        case ReifiedTypeId::Set:
            out << "repeated " << walkType(*type.args[0], ctx);
            break;

        case ReifiedTypeId::Map:
            out << "map<" << walkType(*type.args[0], ctx) << ", " << walkType(*type.args[1], ctx)
                << ">";
            break;

        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0], ctx);
            break;

        default:
            out << "bytes";
            break;
        }

        return out.str();
    }

    std::string generateStructRefType(const StructRefType& type, const WalkContext& ctx) override
    {
        return type.srcTypeString;
    }

    std::string generateStructType(const StructType& type, const WalkContext& ctx) override
    {
        if (type.value && !type.value->name.empty())
        {
            return type.value->name;
        }
        return "bytes";
    }

  private:
    std::string canonicalToProtobuf(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
        case ReifiedTypeId::Int16:
        case ReifiedTypeId::Int32:
            return "int32";
        case ReifiedTypeId::Int64:
            return "int64";
        case ReifiedTypeId::UInt8:
        case ReifiedTypeId::UInt16:
        case ReifiedTypeId::UInt32:
            return "uint32";
        case ReifiedTypeId::UInt64:
            return "uint64";
        case ReifiedTypeId::Float32:
            return "float";
        case ReifiedTypeId::Float64:
            return "double";
        case ReifiedTypeId::String:
            return "string";
        case ReifiedTypeId::Char:
            return "int32";
        case ReifiedTypeId::DateTime:
        case ReifiedTypeId::Date:
        case ReifiedTypeId::Time:
            return "string";
        case ReifiedTypeId::UUID:
            return "string";
        case ReifiedTypeId::Duration:
            return "int64";
        default:
            return "bytes";
        }
    }
};
} // namespace bhw