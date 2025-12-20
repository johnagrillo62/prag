// protobuf_generator.h - Protocol Buffers code generator using AST walker
#pragma once
#include <stack>

#include "ast_walker.h"
namespace bhw
{
class ProtoBufAstWalker : public AstWalker
{
  public:
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

    std::string generateNamespaceOpen(const bhw::Namespace&, size_t) override
    {
        return "";
    }

    std::string generateNamespaceClose(const bhw::Namespace&, size_t) override
    {
        return "";
    }

    std::string generateStructOpen(const Struct& s, size_t level) override
    {
        std::ostringstream out;

        field_number_stack_.push(field_number_);
        field_number_ = 1;

        out << indent(level) << "message " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct&, size_t level) override
    {
        // RESTORE parent's counter when leaving nested scope
        if (!field_number_stack_.empty())
        {
            field_number_ = field_number_stack_.top();
            field_number_stack_.pop();
        }
        return indent(level) + "}\n\n";
    }
    std::string generateField(const Field& field, size_t level) override
    {
        std::ostringstream out;

        // Check if this field is a Variant type -> generate oneof
        if (std::holds_alternative<GenericType>(field.type->value))
        {
            const auto& gt = std::get<GenericType>(field.type->value);
            if (gt.reifiedType == ReifiedTypeId::Variant)
            {
                out << indent(level) << "oneof " << field.name << " {\n";

                int oneof_field_num = field_number_;
                for (const auto& arg : gt.args)
                {
                    std::string typeName = walkType(*arg);
                    std::string fieldName = field.name + "_" + typeName;
                    out << indent(level + 1) << typeName << " " << fieldName << " = "
                        << oneof_field_num++ << ";\n";
                }

                out << indent(level) << "}\n";
                field_number_ = oneof_field_num;
                return out.str();
            }
        }

        // Check if this field has an anonymous struct type
        if (std::holds_alternative<StructType>(field.type->value))
        {
            const auto& structType = std::get<StructType>(field.type->value);
            const Struct& s = *structType.value;

            if (s.isAnonymous || s.name == "<anonymous>")
            {
                std::string messageName = field.name;
                if (!messageName.empty() && std::islower(messageName[0]))
                {
                    messageName[0] = static_cast<char>(std::toupper(messageName[0]));
                }

                int savedFieldNumber = field_number_;
                field_number_ = 1;

                out << indent(level) << "message " << messageName << " {\n";
                for (const auto& member : s.members)
                {
                    if (std::holds_alternative<Field>(member))
                    {
                        const auto& nestedField = std::get<Field>(member);
                        out << generateField(nestedField, level + 1);
                    }
                }

                out << indent(level) << "}\n";

                field_number_ = savedFieldNumber;

                out << indent(level) << messageName << " " << field.name << " = " << field_number_++
                    << ";\n";

                return out.str();
            }
        }

        // Normal field
        out << indent(level) << walkType(*field.type) << " " << field.name << " = "
            << field_number_++ << ";\n";
        return out.str();
    }
    
    std::string generateEnumOpen(const Enum& e, size_t level) override
    {
        std::ostringstream out;
        out << indent(level) << "enum " << e.name << " {\n";

        if (e.values.empty() || e.values[0].number != 0)
        {
            out << indent(level + 1) << e.name << "_UNSPECIFIED = 0;\n";
        }

        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t level) override
    {
        std::ostringstream out;
        out << indent(level) << val.name << " = " << val.number << ";\n";
        return out.str();
    }

    std::string generateEnumClose(const Enum&, size_t level) override
    {
        return indent(level) + "}\n\n";
    }

    std::string generateOneof(const Oneof& oneof, size_t level) override
    {
        std::ostringstream out;
        out << indent(level) << "oneof " << oneof.name << " {\n";

        int oneof_field_num = 1;
        for (const auto& field : oneof.fields)
        {
            out << indent(level + 1) << walkType(*field.type) << " " << field.name << " = "
                << oneof_field_num++ << ";\n";
        }

        out << indent(level) << "}\n";
        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, size_t) override
    {
        return canonicalToProtobuf(type.reifiedType);
    }

    std::string generatePointerType(const PointerType& type, size_t indent = 0) override
    {
        return walkType(*type.pointee, indent);
    }

    std::string generateGenericType(const GenericType& type, size_t indent = 0) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
        case ReifiedTypeId::Set:
            out << "repeated " << walkType(*type.args[0], indent);
            break;

        case ReifiedTypeId::Map:
            out << "map<" << walkType(*type.args[0]) << ", " << walkType(*type.args[1], indent)
                << ">";
            break;

        case ReifiedTypeId::Optional:
            out << walkType(*type.args[0]);
            break;

         default:
            out << "bytes";
            break;
        }

        return out.str();
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return type.value->name;
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