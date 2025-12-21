// jsonschema_walker.h
#pragma once
#include <nlohmann/json.hpp>
#include <stack>

#include "ast_walker.h"

namespace bhw
{

using json = nlohmann::ordered_json;

class JSONSchemaAstWalker : public AstWalker
{
  public:
    Language getLang() override
    {
        return Language::JSONSchema;
    }

  private:
    json defs;
    std::stack<std::string> structStack;
    std::stack<std::vector<std::string>> requiredStack;
    std::vector<std::string> schemaNames;
    int anonCounter = 0;

    std::string generateHeader(const bhw::Ast&) override
    {
        defs = json::object();
        schemaNames.clear();
        anonCounter = 0;
        while (!structStack.empty())
            structStack.pop();
        while (!requiredStack.empty())
            requiredStack.pop();
        return "";
    }

    std::string generateFooter(const bhw::Ast&) override
    {
        // Reorder: enums first, then structs
        json reordered = json::object();
        for (auto& name : schemaNames)
        {
            if (defs[name].contains("enum"))
                reordered[name] = defs[name];
        }
        for (auto& name : schemaNames)
        {
            if (!defs[name].contains("enum"))
                reordered[name] = defs[name];
        }

        json output;
        output["$schema"] = "https://json-schema.org/draft/2020-12/schema";
        output["$defs"] = reordered;
        return output.dump(2) + "\n";
    }

    std::string generateStructOpen(const Struct& s, size_t) override
    {
        std::string name = s.name;
        if (name.empty() || name == "<anonymous>")
        {
            name = "Anonymous" + std::to_string(anonCounter++);
        }

        defs[name] = {{"type", "object"}, {"properties", json::object()}};
        schemaNames.push_back(name);
        structStack.push(name);
        requiredStack.push({});
        return "";
    }

    std::string generateStructClose(const Struct&, size_t) override
    {
        if (structStack.empty())
            return "";

        std::string name = structStack.top();
        auto required = requiredStack.top();

        if (!required.empty())
        {
            defs[name]["required"] = required;
        }

        structStack.pop();
        requiredStack.pop();
        return "";
    }

    std::string generateField(const Field& field, size_t) override
    {
        if (structStack.empty())
            return "";

        std::string structName = structStack.top();

        bool isOptional =
            std::holds_alternative<GenericType>(field.type->value) &&
            std::get<GenericType>(field.type->value).reifiedType == ReifiedTypeId::Optional;

        if (!isOptional)
        {
            requiredStack.top().push_back(field.name);
        }

        defs[structName]["properties"][field.name] = typeToJson(*field.type);
        return "";
    }

    std::string generateEnumOpen(const Enum& e, size_t) override
    {
        defs[e.name] = {{"type", "string"}, {"enum", json::array()}};
        schemaNames.push_back(e.name);
        return "";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t) override
    {
        // Find the last enum added
        for (auto it = schemaNames.rbegin(); it != schemaNames.rend(); ++it)
        {
            if (defs.contains(*it) && defs[*it].contains("enum"))
            {
                defs[*it]["enum"].push_back(val.name);
                break;
            }
        }
        return "";
    }

    std::string generateEnumClose(const Enum&, size_t) override
    {
        return "";
    }

    std::string generateOneof(const Oneof& oneof, size_t) override
    {
        if (structStack.empty())
            return "";

        std::string structName = structStack.top();

        // Create oneOf array with different object schemas for each alternative
        json oneOfArray = json::array();

        // Add null option (representing "not set")
        oneOfArray.push_back({{"type", "null"}});

        // Add each alternative as a separate object schema
        for (const auto& field : oneof.fields)
        {
            // Create wrapper object for this alternative
            std::string wrapperName = capitalize(oneof.name) + "_" + capitalize(field.name);

            json altSchema;
            altSchema["type"] = "object";
            altSchema["properties"] = json::object();
            altSchema["properties"]["value"] = typeToJson(*field.type);
            altSchema["required"] = json::array({"value"});

            // Add to definitions for potential reuse
            defs[wrapperName] = altSchema;
            schemaNames.push_back(wrapperName);

            // Reference in oneOf
            oneOfArray.push_back({{"$ref", "#/$defs/" + wrapperName}});
        }

        // Add the oneOf field to the parent struct
        defs[structName]["properties"][oneof.name] = {{"oneOf", oneOfArray}};

        return "";
    }

    std::string capitalize(const std::string& s)
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(static_cast<unsigned char>(result[0]));
        return result;
    }


    json typeToJson(const Type& type)
    {
        if (std::holds_alternative<SimpleType>(type.value))
        {
            auto& st = std::get<SimpleType>(type.value);
            return primitiveToJson(st.reifiedType);
        }
        if (std::holds_alternative<StructRefType>(type.value))
        {
            auto& ref = std::get<StructRefType>(type.value);
            return {{"$ref", "#/$defs/" + ref.srcTypeString}};
        }
        if (std::holds_alternative<GenericType>(type.value))
        {
            auto& gt = std::get<GenericType>(type.value);
            switch (gt.reifiedType)
            {
            case ReifiedTypeId::List:
                return {{"type", "array"}, {"items", typeToJson(*gt.args[0])}};
            case ReifiedTypeId::Map:
                return {{"type", "object"}, {"additionalProperties", typeToJson(*gt.args[1])}};
            case ReifiedTypeId::Optional:
                return typeToJson(*gt.args[0]);
            case ReifiedTypeId::Variant:
            {
                json oneOf = json::array();
                for (const auto& arg : gt.args)
                {
                    oneOf.push_back(typeToJson(*arg));
                }
                return {{"oneOf", oneOf}};
            }
            default:
                return {{"type", "string"}};
            }
        }
        return {{"type", "string"}};
    }
    
    json primitiveToJson(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return {{"type", "boolean"}};
        case ReifiedTypeId::Int8:
        case ReifiedTypeId::Int16:
        case ReifiedTypeId::Int32:
        case ReifiedTypeId::Int64:
        case ReifiedTypeId::UInt8:
        case ReifiedTypeId::UInt16:
        case ReifiedTypeId::UInt32:
        case ReifiedTypeId::UInt64:
            return {{"type", "integer"}};
        case ReifiedTypeId::Float32:
        case ReifiedTypeId::Float64:
            return {{"type", "number"}};
        case ReifiedTypeId::String:
            return {{"type", "string"}};
        case ReifiedTypeId::Bytes:
            return {{"type", "string"}};
        default:
            return {{"type", "string"}};
        }
    }
};

} // namespace bhw
