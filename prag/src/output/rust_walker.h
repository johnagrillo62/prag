#pragma once
#include "registry_ast_walker.h"

namespace bhw
{
class RustAstWalker : public RegistryAstWalker
{
  public:
    RustAstWalker() : RegistryAstWalker(bhw::Language::Rust)
    {
    }
    Language getLang() override
    {
        return bhw::Language::Rust;
    }

    // Pre-process AST to hoist nested structs
    std::string walk(bhw::Ast&& ast) override
    {
        liftVariantsToEnums(ast);
        flattenNestedStructs(ast);
        return RegistryAstWalker::walk(std::move(ast));
    }

    std::string generateHeader(const bhw::Ast& ast) override
    {
        std::ostringstream out;
        out << "#![allow(dead_code)]\n\n";
        return out.str();
    }

    std::string generateStructOpen(const Struct& s, size_t ind) override
    {
        std::ostringstream out;

        out << indent(ind) << "#[derive(Debug, Clone)]\n";
        out << indent(ind) << "pub struct " << s.name << " {\n";
        return out.str();
    }

    std::string generateStructClose(const Struct& s, size_t ind) override
    {
        return indent(ind) + "}\n\n";
    }

    std::string generateField(const Field& field, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "pub " << field.name << ": " << walkType(*field.type, ind) << ",\n";
        return out.str();
    }

    std::string generateEnumOpen(const Enum& e, size_t ind) override
    {
        std::ostringstream out;

        // Check if this is a variant-style enum (has types associated with values)
        bool hasAssociatedTypes = false;
        for (const auto& val : e.values)
        {
            if (val.type)
            {
                hasAssociatedTypes = true;
                break;
            }
        }

        out << indent(ind) << "#[derive(Debug, Clone";
        if (!hasAssociatedTypes)
        {
            out << ", Copy, PartialEq, Eq";
        }
        out << ")]\n";
        out << indent(ind) << "pub enum " << e.name << " {\n";
        return out.str();
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t ind) override
    {
        std::ostringstream out;

        // If the enum value has an associated type, generate Rust-style variant with data
        if (val.type)
        {
            out << indent(ind) << val.name << "(" << walkType(*val.type, ind) << "),\n";
        }
        else
        {
            // Otherwise, generate C-style enum value
            out << indent(ind) << val.name << " = " << val.number << ",\n";
        }

        return out.str();
    }

    std::string generateEnumClose(const Enum& e, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "}\n\n";
        return out.str();
    }

    std::string generateNamespaceOpen(const Namespace& ns, size_t ind) override
    {
        return indent(ind) + "pub mod " + ns.name + " {\n";
    }

    std::string generateNamespaceClose(const Namespace& ns, size_t ind) override
    {
        return indent(ind) + "} // mod " + ns.name + "\n\n";
    }

    std::string generatePointerType(const PointerType& type, size_t ind = 0) override
    {
        return "Box<" + walkType(*type.pointee, ind) + ">";
    }

    std::string generateStructType(const StructType& type, size_t) override
    {
        return type.value->name;
    }

    std::string generateGenericType(const GenericType& type, size_t ind) override
    {
        std::ostringstream out;

        switch (type.reifiedType)
        {
        case ReifiedTypeId::List:
        case ReifiedTypeId::Set:
            out << "Vec<" << walkType(*type.args[0], ind) << ">";
            break;

        case ReifiedTypeId::Map:
            out << "std::collections::HashMap<" << walkType(*type.args[0], ind) << ", "
                << walkType(*type.args[1], ind) << ">";
            break;

        case ReifiedTypeId::Optional:
            out << "Option<" << walkType(*type.args[0], ind) << ">";
            break;

        default:
            out << "Vec<u8>";
            break;
        }

        return out.str();
    }

    std::string generateSimpleType(const SimpleType& type, size_t) override
    {
        return canonicalToRust(type.reifiedType);
    }

    std::string generateOneof(const Oneof& oneof, size_t ind) override
    {
        std::ostringstream out;
        out << indent(ind) << "#[derive(Debug, Clone)]\n";
        out << indent(ind) << "pub enum " << oneof.name << " {\n";

        for (const auto& field : oneof.fields)
        {
            out << indent(ind + 1) << field.name << "(" << walkType(*field.type, ind) << "),\n";
        }

        out << indent(ind) << "}\n\n";
        return out.str();
    }

  private:
    std::string canonicalToRust(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
            return "i8";
        case ReifiedTypeId::Int16:
            return "i16";
        case ReifiedTypeId::Int32:
            return "i32";
        case ReifiedTypeId::Int64:
            return "i64";
        case ReifiedTypeId::UInt8:
            return "u8";
        case ReifiedTypeId::UInt16:
            return "u16";
        case ReifiedTypeId::UInt32:
            return "u32";
        case ReifiedTypeId::UInt64:
            return "u64";
        case ReifiedTypeId::Float32:
            return "f32";
        case ReifiedTypeId::Float64:
            return "f64";
        case ReifiedTypeId::String:
            return "String";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::Bytes:
            return "Vec<u8>";
        default:
            return "Vec<u8>";
        }
    }

    void flattenNestedStructs(bhw::Ast& ast)
    {
        std::vector<bhw::AstRootNode> hoistedNodes;

        for (auto& node : ast.nodes)
        {
            if (auto* s = std::get_if<bhw::Struct>(&node))
            {
                flattenStructMembers(*s, s->name, hoistedNodes);
            }
        }

        // Insert hoisted nodes at the beginning
        for (auto& hoisted : hoistedNodes)
        {
            ast.nodes.insert(ast.nodes.begin(), std::move(hoisted));
        }
    }

    void flattenStructMembers(bhw::Struct& s,
                              const std::string& parentName,
                              std::vector<bhw::AstRootNode>& hoistedNodes)
    {
        std::vector<bhw::StructMember> newMembers;

        for (auto& member : s.members)
        {
            if (auto* field = std::get_if<bhw::Field>(&member))
            {
                flattenFieldType(*field, parentName, hoistedNodes);
                newMembers.push_back(std::move(member));
            }
            else if (auto* nestedStruct = std::get_if<bhw::Struct>(&member))
            {
                // Flatten ALL nested structs with variable names (Rust doesn't support any)
                if (!nestedStruct->variableName.empty())
                {
                    std::string structName;

                    // Determine struct name
                    bool isAnonymous = nestedStruct->isAnonymous || nestedStruct->name.empty() ||
                                       nestedStruct->name.find("anonymous") != std::string::npos;

                    if (!isAnonymous)
                    {
                        // Named nested struct - use its existing name
                        structName = nestedStruct->name;
                    }
                    else
                    {
                        // Anonymous nested struct - generate name from variable
                        structName = parentName + "_";
                        structName += static_cast<char>(std::toupper(
                            static_cast<unsigned char>(nestedStruct->variableName[0])));
                        if (nestedStruct->variableName.length() > 1)
                        {
                            structName += nestedStruct->variableName.substr(1);
                        }
                    }

                    // Recursively flatten this struct's members
                    flattenStructMembers(*nestedStruct, structName, hoistedNodes);

                    // Create hoisted struct
                    bhw::Struct hoistedStruct;
                    hoistedStruct.name = structName;
                    hoistedStruct.isAnonymous = false;
                    hoistedStruct.namespaces = nestedStruct->namespaces;
                    hoistedStruct.attributes = nestedStruct->attributes;

                    for (auto& nestedMember : nestedStruct->members)
                    {
                        hoistedStruct.members.push_back(std::move(nestedMember));
                    }

                    // Replace with a Field referencing the struct
                    bhw::Field field;
                    field.name = nestedStruct->variableName;
                    bhw::StructRefType structRef;
                    structRef.srcTypeString = structName;
                    structRef.reifiedType = ReifiedTypeId::StructRefType;
                    field.type = std::make_unique<bhw::Type>(structRef);

                    newMembers.push_back(std::move(field));
                    hoistedNodes.push_back(std::move(hoistedStruct));
                }
                else
                {
                    // No variable name - this is a type definition, not a field
                    // Recursively flatten this struct's members
                    flattenStructMembers(*nestedStruct, nestedStruct->name, hoistedNodes);

                    // Create hoisted struct
                    bhw::Struct hoistedStruct;
                    hoistedStruct.name = nestedStruct->name;
                    hoistedStruct.isAnonymous = false;
                    hoistedStruct.namespaces = nestedStruct->namespaces;
                    hoistedStruct.attributes = nestedStruct->attributes;

                    for (auto& nestedMember : nestedStruct->members)
                    {
                        hoistedStruct.members.push_back(std::move(nestedMember));
                    }

                    hoistedNodes.push_back(std::move(hoistedStruct));
                    // Don't add to newMembers - this is a type definition, not a field
                }
            }
            else if (auto* nestedEnum = std::get_if<bhw::Enum>(&member))
            {
                // Hoist enums to top level
                hoistedNodes.push_back(std::move(*nestedEnum));
                // Don't add to newMembers - enums aren't fields
            }
            else
            {
                // Keep other members (Oneof, etc.)
                newMembers.push_back(std::move(member));
            }
        }

        s.members = std::move(newMembers);
    }

    void flattenFieldType(bhw::Field& field,
                          const std::string& parentName,
                          std::vector<bhw::AstRootNode>& hoistedNodes)
    {
        // Handle direct StructType
        if (std::get_if<bhw::StructType>(&field.type->value))
        {
            flattenIfAnonymous(field.type, parentName, field.name, hoistedNodes);
        }

        else if (auto* genericType = std::get_if<bhw::GenericType>(&field.type->value))
        {
            for (auto& arg : genericType->args)
            {
                if (std::get_if<bhw::StructType>(&arg->value))
                {
                    flattenIfAnonymous(arg, parentName, field.name, hoistedNodes);
                }
            }
        }
    }

    void flattenIfAnonymous(std::unique_ptr<bhw::Type>& typePtr,
                            const std::string& parentName,
                            const std::string& fieldName,
                            std::vector<bhw::AstRootNode>& hoistedNodes)
    {
        auto* structType = std::get_if<bhw::StructType>(&typePtr->value);
        if (!structType)
            return;

        bhw::Struct* anonymousStruct = structType->value.get();

        // Check if it's anonymous
        bool isAnonymous = anonymousStruct->isAnonymous || anonymousStruct->name.empty() ||
                           anonymousStruct->name.find("anonymous") != std::string::npos;

        if (!isAnonymous)
            return;

        // Generate a name: ParentName_FieldName
        std::string generatedName = parentName + "_";
        if (!fieldName.empty())
        {
            generatedName +=
                static_cast<char>(std::toupper(static_cast<unsigned char>(fieldName[0])));
            if (fieldName.length() > 1)
            {
                generatedName += fieldName.substr(1);
            }
        }

        // Recursively flatten nested anonymous structs FIRST
        flattenStructMembers(*anonymousStruct, generatedName, hoistedNodes);

        // Create a new struct to hoist by manually copying members
        bhw::Struct hoistedStruct;
        hoistedStruct.name = generatedName;
        hoistedStruct.isAnonymous = false;
        hoistedStruct.namespaces = anonymousStruct->namespaces;
        hoistedStruct.attributes = anonymousStruct->attributes;

        // Move members to avoid copying issues
        for (auto& member : anonymousStruct->members)
        {
            hoistedStruct.members.push_back(std::move(member));
        }

        // Replace the StructType with a StructRef
        bhw::StructRefType structRef;
        structRef.srcTypeString = generatedName;
        structRef.reifiedType = ReifiedTypeId::StructRefType;
        typePtr->value = structRef;

        // Hoist the struct
        hoistedNodes.push_back(std::move(hoistedStruct));
    }

    // -------------------------------------------------------------------
    void liftVariantsToEnums(Ast& ast)
    {
        std::vector<Enum> liftedEnums;

        // Process all top-level structs
        for (auto& node : ast.nodes)
        {
            if (auto* s = std::get_if<Struct>(&node))
            {
                liftStructVariants(*s, liftedEnums);
            }
        }

        // Insert all lifted enums at top-level (at the BEGINNING so they're defined before use)
        // Insert in reverse order to maintain correct order
        for (auto it = liftedEnums.rbegin(); it != liftedEnums.rend(); ++it)
        {
            ast.nodes.insert(ast.nodes.begin(), std::move(*it));
        }
    }

    // -------------------------------------------------------------------
    void liftStructVariants(Struct& s, std::vector<Enum>& liftedEnums)
    {
        std::vector<StructMember> newMembers;

        for (auto& member : s.members)
        {
            if (auto* field = std::get_if<Field>(&member))
            {
                if (field->type && field->type->isGeneric())
                {
                    auto& gen = std::get<GenericType>(field->type->value);
                    if (gen.reifiedType == ReifiedTypeId::Variant)
                    {
                        // Create a new Enum from the Variant field
                        Enum e;
                        e.name = field->name;        // enum name = field name
                        e.namespaces = s.namespaces; // inherit struct's namespace

                        for (size_t i = 0; i < gen.args.size(); ++i)
                        {
                            EnumValue ev;
                            ev.name = "Variant" + std::to_string(i);
                            ev.number = static_cast<int>(i);  // Set proper index
                            ev.type = std::move(gen.args[i]); // move the type
                            e.values.push_back(std::move(ev));
                        }

                        // Save the enum name BEFORE moving
                        std::string enumName = e.name;

                        liftedEnums.push_back(std::move(e));

                        // Replace field type with StructRefType pointing to new enum
                        StructRefType ref;
                        ref.srcTypeString = enumName; // Use saved name, not e.name
                        ref.reifiedType = ReifiedTypeId::StructRefType;
                        field->type = std::make_unique<Type>(ref);

                        // Add field only, do NOT add the enum itself
                        newMembers.push_back(std::move(*field));
                        continue;
                    }
                }
                newMembers.push_back(std::move(member));
            }
            else if (auto* oneof = std::get_if<Oneof>(&member))
            {
                // Lift Oneof to top-level enum
                Enum e;
                e.name = oneof->name;
                e.namespaces = s.namespaces;

                // Convert Oneof fields to EnumValues
                for (size_t i = 0; i < oneof->fields.size(); ++i)
                {
                    EnumValue ev;
                    ev.name = oneof->fields[i].name;
                    ev.number = static_cast<int>(i);
                    ev.type = std::move(oneof->fields[i].type);
                    e.values.push_back(std::move(ev));
                }

                // Save the enum name
                std::string enumName = e.name;

                liftedEnums.push_back(std::move(e));

                // Replace Oneof with a Field referencing the enum
                Field field;
                field.name = enumName;
                StructRefType ref;
                ref.srcTypeString = enumName;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                field.type = std::make_unique<Type>(ref);

                newMembers.push_back(std::move(field));
            }
            else if (auto* nestedStruct = std::get_if<Struct>(&member))
            {
                // Recurse into nested structs
                liftStructVariants(*nestedStruct, liftedEnums);
                newMembers.push_back(std::move(member));
            }
            else
            {
                // Keep other members
                newMembers.push_back(std::move(member));
            }
        }

        s.members = std::move(newMembers);
    }
};

} // namespace bhw
